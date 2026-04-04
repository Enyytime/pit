#ifndef HASH_OBJECT_H
#define HASH_OBJECT_H

#include <zlib.h>

char* get_full_directory_name(const char* hex);
char* convert_hash_to_string(unsigned char* hash);
unsigned char* compress_data(unsigned char* data, uLongf original_size, uLongf* compressed_size);
void hash_file(const char* filename);


#endif
