#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "include/entry.h"

static EntryNode* entries;
static int entry_count = 0;


/**
 * @brief Adds a new entry to the in-memory staging list.
 *
 * Reallocates the entries array and appends a new EntryNode
 * with the given mode, hash, and filename.
 *
 * @param mode      File mode string (e.g. "100644")
 * @param hash      40-char hex SHA1 hash of the blob
 * @param filename  Path to the file being staged
 */
void add_entry(const char* mode, const char* hash, const char* filename){
    entries = realloc(entries, sizeof(EntryNode) * (entry_count + 1));
    entries[entry_count].mode = strdup(mode);
    entries[entry_count].hash = strdup(hash);
    entries[entry_count].filename = strdup(filename);
    entry_count++; 
}   

/**
 * @brief Returns the in-memory staging list and its size.
 *
 * @param count  Output: number of entries in the list
 * @return       Pointer to the EntryNode array
 */
EntryNode* get_entries(int* count){
    *count = entry_count;
    return entries;
}

