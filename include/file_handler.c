#include "file_handler.h"
#include <stdlib.h>

/**
 * @brief Opens a file in binary read mode.
 *
 * @param filename  Path to the file to open
 * @return          FILE* handle, or exits with error if file not found
 */
FILE* open_file(const char* filename){
    FILE* file = fopen(filename, "rb");
    if(file == NULL){
        fprintf(stderr, "file can't be openned\n");
        exit(1);
    }

    return file;
}

/**
 * @brief Reads the entire file content into a heap-allocated buffer.
 *
 * @param fileStruct  FileStruct with an open file handle and filesize
 * @return            Heap-allocated buffer containing file bytes — caller must free
 */
unsigned char* read_file_to_string(FileStruct fileStruct){
    unsigned char *buf = (unsigned char*)malloc(fileStruct.filesize);
    fread(buf, 1, fileStruct.filesize, fileStruct.file);
    return buf;
}

void write_file(const char* path, unsigned char* data, size_t size){
    FILE* out = fopen(path, "wb");
    if(out == NULL){
        fprintf(stderr, "could not write to %s\n", path);
        exit(1);
    }
    fwrite(data, 1, size, out);
    fclose(out);
}

/**
 * @brief Opens a file and initializes a FileStruct with its handle and size.
 *
 * Uses fseek/ftell to determine the file size, then rewinds to the start.
 *
 * @param filename  Path to the file
 * @return          Initialized FileStruct with open file handle and size
 */
FileStruct init_file_struct(const char* filename){
    FileStruct f;

    f.file = open_file(filename);

    // seek to end to get file size, then rewind
    fseek(f.file, 0, SEEK_END);
    long filesize = ftell(f.file);
    fseek(f.file, 0, SEEK_SET);

    f.filesize = filesize;
    return f;
}
