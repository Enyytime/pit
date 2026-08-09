#include <openssl/sha.h>   // SHA1
#include <zlib.h>          // compress
#include <stdio.h>
#include "include/file_handler.h"
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h> 
#include "include/init.h"

/**
 * @brief Decompresses a zlib-compressed buffer.
 *
 * Allocates a buffer 4x the compressed size as an estimate for the
 * decompressed output. Highly compressible data can exceed that
 * estimate, in which case zlib reports Z_BUF_ERROR and this returns
 * NULL rather than a partially filled buffer.
 *
 * @param decompressed_size  Output: actual size of decompressed data
 * @param f                  FileStruct used to determine compressed size
 * @param compress           Compressed data buffer
 * @return                   Heap-allocated decompressed buffer, or NULL
 *                           on failure — caller must free
 */
unsigned char* decompress_data(uLongf* decompressed_size, FileStruct f, unsigned char* compress){
    *decompressed_size = f.filesize * 4;
    unsigned char* decompressed = (unsigned char*)malloc(*decompressed_size);
    int return_status = uncompress(decompressed, decompressed_size, compress, f.filesize);

    if (return_status != Z_OK) {
        free(decompressed);
        return NULL;
    }
    return decompressed;
}


/**
 * @brief Reads and prints the contents of a pit object by hash.
 *
 * Builds the object path from the hash, reads and decompresses it,
 * strips the blob header, and prints the file content to stdout.
 *
 * @param hash  40-char hex SHA1 hash of the object
 */
char* cat_file(const char* hash, int* size){
    
    char* path = (char*)malloc(128);
    snprintf(path, 128, ".pit/objects/%.2s/%s", hash, hash + 2);

    FileStruct file_struct = init_file_struct(path);
    unsigned char* compressed = read_file_to_string(file_struct);

    uLongf decompressed_size;
    unsigned char* decompressed = decompress_data(&decompressed_size, file_struct, compressed);
    
    // skip the header
    unsigned char *null_pos = memchr(decompressed, '\0', decompressed_size);
    unsigned char *content = null_pos + 1;

    if (size != NULL) {
        *size = decompressed_size - (content - decompressed);
    }

    memmove(decompressed, content, decompressed_size - (content - decompressed));
    return (char*)decompressed;
}

void pit_cat_file(const char* hash){
    char* content = cat_file(hash, NULL);
    printf("%s\n", content);
}