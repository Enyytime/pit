#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <stdio.h>

/**
 * @brief Holds a file pointer and its size in bytes.
 */
typedef struct {
    FILE* file;     /**< Open file handle */
    int filesize;   /**< Size of the file in bytes */
} FileStruct;

FILE* open_file(const char* filename);
unsigned char* read_file_to_string(FileStruct fileStruct);
FileStruct init_file_struct(const char* filename);
void write_file(const char* path, unsigned char* data, size_t size);

#endif
