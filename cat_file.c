#include <openssl/sha.h>   // SHA1
#include <zlib.h>          // compress
#include <stdio.h>
#include "include/file_handler.h"
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h> 
#include "include/init.h"

unsigned char* decompress_data(uLongf* decompressed_size, FileStruct f, unsigned char* compress){
    *decompressed_size = f.filesize * 4;
    unsigned char* decompressed = (unsigned char*)malloc(*decompressed_size);
    uncompress(decompressed, decompressed_size, compress, f.filesize);
    return decompressed;
}


void cat_file(const char* hash){
    char* path = (char*)malloc(128);
    snprintf(path, 128, ".pit/objects/%.2s/%s", hash, hash + 2);

    FileStruct file_struct = init_file_struct(path);
    unsigned char* compressed = read_file_to_string(file_struct);

    uLongf decompressed_size;
    unsigned char* decompressed = decompress_data(&decompressed_size, file_struct, compressed);
    
    // skip the header
    unsigned char *content = memchr(decompressed, '\0', decompressed_size);
    content++; // move the pointer after the \0;

    printf("%s", (char*)content);
}