#ifndef HASH_OBJECT_H
#define HASH_OBJECT_H

#include <zlib.h>

char* get_full_directory_name(const char* hex);
char* convert_hash_to_string(unsigned char* hash);
unsigned char* compress_data(unsigned char* data, uLongf original_size, uLongf* compressed_size);
char* hash_file(const char* filename);
void cmd_hash_file(const char* filename);
char* store_object(const char* type, unsigned char* data, int size);
#endif
