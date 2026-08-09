/**
 * im still trying to understand how packfiles works so for this file i ask
 * AI to fully write it
 * 
 */

#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include "include/pack.h"
#include "include/file_handler.h"

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