#include <openssl/sha.h>   // SHA1
#include <zlib.h>          // compress
#include <stdio.h>
#include "include/file_handler.h"
#include <string.h>
#include <stdlib.h>
#include "include/hash_object.h"
#include <sys/stat.h> 

/**
 * @brief Builds the full directory path for a git object.
 *
 * Git splits the 40-char hex hash into a 2-char directory name and
 * a 38-char filename to avoid too many files in one directory.
 *
 * @param hex  40-char null-terminated hex string of the SHA1 hash
 * @return     Heap-allocated string like ".pit/objects/af" — caller must free
 */
char* get_full_directory_name (const char* hex) {
    // take first 2 chars of hex as the directory name
    char *dirName = strndup(hex, 2);
    char* dir = (char*)malloc(sizeof(char) * 64);

    // build full path: .pit/objects/<2-char-prefix>
    snprintf(dir, 64, ".pit/objects/%s", dirName);
    free(dirName);
    return dir; // caller must free
}

/**
 * @brief Converts a SHA1 hash byte array to a hex string.
 *
 * SHA1 produces 20 bytes. Each byte is printed as 2 hex chars,
 * resulting in a 40-char string.
 *
 * @param hash  20-byte SHA1 hash (SHA_DIGEST_LENGTH)
 * @return      Heap-allocated 41-char null-terminated hex string — caller must free
 */
char* convert_hash_to_string (unsigned char* hash) {
    char* hex = (char*)malloc(sizeof(char) * 41);

    // convert each byte to 2 hex characters
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        sprintf(hex + (i * 2), "%02x", hash[i]);
    }
    hex[40] = '\0';

    return hex; // caller must free
}

/**
 * @brief Compresses data using zlib.
 *
 * @param data             Raw data buffer to compress
 * @param original_size    Size of the input data in bytes
 * @param compressed_size  Output: size of the compressed data
 * @return                 Heap-allocated compressed buffer — caller must free
 */
unsigned char* compress_data (unsigned char* data, uLongf original_size, uLongf* compressed_size) {
    *compressed_size = compressBound(original_size);
    unsigned char *compressed = (unsigned char*)malloc(*compressed_size);
    compress(compressed, compressed_size, data, original_size);
    return compressed;
}   


/**
 * @brief Hashes a file and stores it as a blob object in .pit/objects/.
 *
 * Reads the file, builds a git blob header ("blob <size>\0"),
 * concatenates header + content, SHA1 hashes the result,
 * and stores the zlib-compressed data at .pit/objects/<2-char>/<38-char>.
 *
 * @param filename  Path to the file to hash
 */
void hash_file (const char* filename) {

    // open file and get its size
    FileStruct fileStruct = init_file_struct(filename);

    // build blob header: "blob <filesize>\0"
    char header[32];
    int header_len = snprintf(header, sizeof(header), "blob %d", fileStruct.filesize) + 1; // +1 to include the \0

    // read file content into buffer
    unsigned char *content = read_file_to_string(fileStruct);

    // allocate buffer for header + content combined
    unsigned char *full = malloc(header_len + fileStruct.filesize);

    // copy header then content into the combined buffer
    memcpy(full, header, header_len);
    memcpy(full + header_len, content, fileStruct.filesize);

    // SHA1 hash the combined buffer
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(full, header_len + fileStruct.filesize, hash);

    // convert hash bytes to 40-char hex string
    char* hex = convert_hash_to_string(hash);

    // build the object directory path (.pit/objects/<first-2-chars>)
    char* dir = get_full_directory_name(hex);
 
    if(mkdir(dir, 0755) == -1){
        perror(dir);
    }

    char* path = (char*)malloc(128);
    snprintf(path, 128, "%s/%s", dir, hex + 2);

    // compress
    uLongf compressed_size;
    unsigned char *compressed = compress_data(full, header_len + fileStruct.filesize, &compressed_size);

    // write to file
    write_file(path, compressed, compressed_size);

    printf("%s\n", hex);
}
