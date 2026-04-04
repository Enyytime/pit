#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "include/entry.h"

static EntryNode* entries;
static int entry_count = 0;


void add_entry(const char* mode, const char* hash, const char* filename){
    entries = realloc(entries, sizeof(EntryNode) * (entry_count + 1));
    entries[entry_count].mode = strdup(mode);
    entries[entry_count].hash = strdup(hash);
    entries[entry_count].filename = strdup(filename);
    entry_count++; 
}   

EntryNode* get_entries(int* count){
    *count = entry_count;
    return entries;
}

