#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <openssl/sha.h>
#include "include/pack.h"
#include "include/file_handler.h"
#include "include/hash_object.h"

/*
PACK<version><count>            <- 12 byte header
<object header><zlib data>      <- repeated count times
<20 byte sha1 of everything above>
*/

/**
 * @brief Turns an object type number into a printable name.
 *
 * @param type  Type number taken from an object header
 * @return      Static string, do not free
 */
const char* get_type_name(int type){
    if(type == OBJ_COMMIT){
        return "commit";
    }
    if(type == OBJ_TREE){
        return "tree";
    }
    if(type == OBJ_BLOB){
        return "blob";
    }
    if(type == OBJ_TAG){
        return "tag";
    }
    if(type == OBJ_OFS_DELTA){
        return "ofs_delta";
    }
    if(type == OBJ_REF_DELTA){
        return "ref_delta";
    }
    return "unknown";
}

/**
 * @brief Turns a type name into the number used in a pack header.
 *
 * @param type_name  "commit", "tree", "blob" or "tag"
 * @return           The type number, or 0 if the name is not known
 */
int get_type_number(const char* type_name){
    if(!strcmp(type_name, "commit")){
        return OBJ_COMMIT;
    }
    if(!strcmp(type_name, "tree")){
        return OBJ_TREE;
    }
    if(!strcmp(type_name, "blob")){
        return OBJ_BLOB;
    }
    if(!strcmp(type_name, "tag")){
        return OBJ_TAG;
    }
    return 0;
}

/**
 * @brief Reads a 4 byte number stored biggest byte first.
 *
 * Packfiles store numbers the opposite way round to x86, so the bytes
 * have to be put back together by hand instead of just copying them
 * into an int.
 *
 * @param buf  Points at the first of the 4 bytes
 * @return     The number
 */
int read_big_endian(unsigned char* buf){
    return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
}

/**
 * @brief Writes a 4 byte number biggest byte first.
 *
 * The opposite of read_big_endian. Shifts each byte out of the number
 * instead of shifting it in.
 *
 * @param buf     Where to write the 4 bytes
 * @param number  The number to write
 */
void write_big_endian(unsigned char* buf, int number){
    buf[0] = (number >> 24) & 255;
    buf[1] = (number >> 16) & 255;
    buf[2] = (number >> 8) & 255;
    buf[3] = number & 255;
}

/**
 * @brief Reads one object's type and uncompressed size.
 *
 * The size is spread over however many bytes it needs. The first byte
 * holds a "keep going" flag in the top bit, the type in the next three
 * bits, and the bottom 4 bits of the size. Every byte after that adds
 * 7 more bits of size, each one more significant than the last.
 *
 * @param buf   Points at the start of the object header
 * @param type  Output: the object type
 * @param size  Output: the uncompressed size
 * @return      How many bytes the header used
 */
int read_object_header(unsigned char* buf, int* type, int* size){
    int pos = 0;
    unsigned char byte = buf[pos];
    pos++;

    *type = (byte >> 4) & 7;    // bits 4,5,6
    *size = byte & 15;          // bits 0,1,2,3

    int shift = 4;
    while(byte & 128){          // top bit set means another byte follows
        byte = buf[pos];
        pos++;
        *size = *size | ((byte & 127) << shift);
        shift = shift + 7;
    }

    return pos;
}

/**
 * @brief Writes one object's type and uncompressed size.
 *
 * The opposite of read_object_header. The first byte takes the type
 * and the bottom 4 bits of the size, then 7 bits of size go in each
 * byte after that. Every byte except the last gets its top bit set so
 * the reader knows to keep going.
 *
 * @param buf   Where to write the header, needs room for 8 bytes
 * @param type  The object type
 * @param size  The uncompressed size
 * @return      How many bytes the header used
 */
int write_object_header(unsigned char* buf, int type, int size){
    int pos = 0;

    unsigned char byte = (type << 4) | (size & 15);
    size = size >> 4;
    if(size > 0){
        byte = byte | 128;
    }
    buf[pos] = byte;
    pos++;

    while(size > 0){
        byte = size & 127;
        size = size >> 7;
        if(size > 0){
            byte = byte | 128;
        }
        buf[pos] = byte;
        pos++;
    }

    return pos;
}

/**
 * @brief Decompresses one object and reports how much input it used.
 *
 * uncompress() can't be used here. Objects sit right next to each other
 * with nothing marking where one ends, so the only way to find the next
 * one is to ask zlib how many compressed bytes it read. inflate() keeps
 * that in total_in, uncompress() throws it away.
 *
 * @param in        Points at the compressed data
 * @param in_size   How many bytes are left in the pack from here
 * @param out_size  Uncompressed size, taken from the object header
 * @param used      Output: how many compressed bytes were read
 * @return          Heap buffer with the object bytes — caller must free
 */
unsigned char* inflate_object(unsigned char* in, int in_size, int out_size, int* used){
    unsigned char* out = (unsigned char*)malloc(out_size + 1);
    if(out == NULL){
        return NULL;
    }

    z_stream stream;
    memset(&stream, 0, sizeof(stream));
    stream.next_in = in;
    stream.avail_in = in_size;
    stream.next_out = out;
    stream.avail_out = out_size;

    if(inflateInit(&stream) != Z_OK){
        free(out);
        return NULL;
    }

    int zlib_status = inflate(&stream, Z_FINISH);
    if(zlib_status != Z_STREAM_END){
        inflateEnd(&stream);
        free(out);
        return NULL;
    }

    *used = stream.total_in;
    inflateEnd(&stream);
    out[out_size] = '\0';
    return out;
}

/**
 * @brief Reads every object out of a packfile.
 *
 * Delta objects are returned with their instructions still packed, not
 * applied. Their base reference is skipped over so the offsets of the
 * objects after them stay correct.
 *
 * @param path   Path to the .pack file
 * @param count  Output: how many objects were read
 * @return       Heap array of PackObject — caller must free with free_pack
 */
PackObject* read_pack(const char* path, int* count){
    *count = 0;

    FileStruct file_struct = init_file_struct(path);
    unsigned char* data = read_file_to_string(file_struct);
    fclose(file_struct.file);

    int filesize = file_struct.filesize;

    if(filesize < 32 || memcmp(data, "PACK", 4) != 0){
        fprintf(stderr, "not a packfile\n");
        free(data);
        return NULL;
    }

    int version = read_big_endian(data + 4);
    int total = read_big_endian(data + 8);

    if(version != 2){
        fprintf(stderr, "unsupported pack version %d\n", version);
        free(data);
        return NULL;
    }

    PackObject* objects = (PackObject*)malloc(sizeof(PackObject) * total);

    int pos = 12;
    int pack_end = filesize - 20;   // last 20 bytes are a checksum, not an object
    int length = 0;

    while(length < total && pos < pack_end){
        int offset = pos;
        int type;
        int size;

        pos = pos + read_object_header(data + pos, &type, &size);

        // deltas name their base before the compressed data starts
        if(type == OBJ_REF_DELTA){
            pos = pos + 20;
        } else if(type == OBJ_OFS_DELTA){
            unsigned char byte = data[pos];
            pos++;
            while(byte & 128){
                byte = data[pos];
                pos++;
            }
        }

        int used;
        unsigned char* content = inflate_object(data + pos, pack_end - pos, size, &used);
        if(content == NULL){
            fprintf(stderr, "could not inflate object at %d\n", offset);
            break;
        }

        objects[length].type = type;
        objects[length].size = size;
        objects[length].offset = offset;
        objects[length].content = content;
        length++;

        pos = pos + used;
    }

    free(data);
    *count = length;
    return objects;
}

/**
 * @brief Writes a list of objects out as a packfile.
 *
 * Every object is stored whole. Git would normally store similar
 * objects as deltas to save space, but a pack with no deltas in it is
 * still valid, just bigger.
 *
 * The whole pack is built in memory first because the trailer is a
 * SHA1 of everything before it, so nothing can be written until all
 * the objects are done.
 *
 * @param path     Where to write the .pack file
 * @param objects  Objects to pack, each needs type, size and content
 * @param count    How many objects to write
 * @return         0 on success, 1 on failure
 */
int write_pack(const char* path, PackObject* objects, int count){
    int capacity = 1024;
    int length = 0;
    unsigned char* pack = (unsigned char*)malloc(capacity);
    if(pack == NULL){
        return 1;
    }

    // 12 byte header: "PACK", version 2, how many objects
    memcpy(pack, "PACK", 4);
    write_big_endian(pack + 4, 2);
    write_big_endian(pack + 8, count);
    length = 12;

    for(int i = 0; i < count; i++){
        uLongf compressed_size;
        unsigned char* compressed = compress_data(objects[i].content,
                                                  objects[i].size,
                                                  &compressed_size);
        if(compressed == NULL){
            free(pack);
            return 1;
        }

        // make sure there is room for the header, the data and the trailer
        while(length + 8 + (int)compressed_size + 20 >= capacity){
            capacity = capacity * 2;
            unsigned char* bigger = (unsigned char*)realloc(pack, capacity);
            if(bigger == NULL){
                free(compressed);
                free(pack);
                return 1;
            }
            pack = bigger;
        }

        length = length + write_object_header(pack + length,
                                              objects[i].type,
                                              objects[i].size);

        memcpy(pack + length, compressed, compressed_size);
        length = length + compressed_size;
        free(compressed);
    }

    // trailer is the sha1 of everything written so far, as raw bytes
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(pack, length, hash);
    memcpy(pack + length, hash, SHA_DIGEST_LENGTH);
    length = length + SHA_DIGEST_LENGTH;

    FILE* file = fopen(path, "wb");
    if(file == NULL){
        perror(path);
        free(pack);
        return 1;
    }
    fwrite(pack, 1, length, file);
    fclose(file);
    free(pack);
    return 0;
}

/**
 * @brief Frees the array returned by read_pack and everything in it.
 *
 * @param objects  Array to free
 * @param count    How many objects it holds
 */
void free_pack(PackObject* objects, int count){
    for(int i = 0; i < count; i++){
        free(objects[i].content);
    }
    free(objects);
}