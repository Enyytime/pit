#ifndef ENTRY_H
#define ENTRY_H

typedef struct entry {
    char* mode;
    char* hash;
    char* filename;    
} EntryNode;

void add_entry(const char* mode, const char* hash, const char* filename);
EntryNode* get_entries(int* count);

#endif
