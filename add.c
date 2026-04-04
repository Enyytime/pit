#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "include/hash_object.h"
#include "include/entry.h"

/**
 * @brief Writes all staged entries to the .pit/index file.
 *
 * Opens .pit/index in append mode and writes each entry as:
 * "<mode> <hash> <filename>"
 *
 * @param entry  Array of EntryNode to write
 * @param count  Number of entries in the array
 */
void put_to_index(EntryNode entry){
    FILE* index_file = fopen(".pit/index", "a");
    if(index_file == NULL){
        perror(".pit/index");
        return;
    }
    // write each entry on its own line
    fprintf(index_file, "%s %s %s\n", entry.mode, entry.hash, entry.filename);

    fclose(index_file);
}

/**
 * @brief Hashes a single file and stages it in the index.
 *
 * Calls hash_file to store the blob, then adds an entry to the
 * in-memory entry list and writes it to .pit/index.
 *
 * @param filename  Path to the file to stage
 */
void handle_one_file(const char* filename){
    // hash the file and store it as a blob object
    char* hashed_file = hash_file(filename);
    char* mode = "100644"; // hardcode for now

    // add to in-memory entry list
    add_entry(mode, hashed_file, filename);

    int entry_count;
    EntryNode* entry = get_entries(&entry_count);

    // write to .pit/index
    put_to_index(entry[entry_count - 1]);
}

/**
 * @brief Handles staging all files in the current directory.
 *
 * Not yet implemented.
 */
void handle_multiple_file(){
    return;
}

/**
 * @brief Entry point for the pit add command.
 * 
 * If filename is "." stages all files, otherwise stages a single file.
 *
 * @param filename  File to stage, or "." for all files
 */
void pit_add(const char* filename){
    if(!strcmp(filename, ".")){
        handle_multiple_file();
    } else {
        handle_one_file(filename);
    }
}
