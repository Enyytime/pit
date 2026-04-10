#ifndef CAT_FILE_H
#define CAT_FILE_H

#include <zlib.h>
#include "file_handler.h"

unsigned char* decompress_data(uLongf* decompressed_size, FileStruct f, unsigned char* compress);

char* cat_file(const char* hash, int* size);
void pit_cat_file(const char* hash);
#endif
