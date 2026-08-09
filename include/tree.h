#ifndef PIT_TREE_H
#define PIT_TREE_H

#include <stdbool.h>
#include <stddef.h>

#ifndef HASH_HEX_LEN
#define HASH_HEX_LEN 40
#endif

typedef struct {
    char mode[8];
    char name[256];
    char hash[HASH_HEX_LEN + 1];
    bool is_dir;
} TreeItem;

typedef void (*TreeVisitor)(const char* path, const TreeItem* item, void* userdata);

TreeItem* parse_tree(const char* tree_hash, int* count);
void walk_tree(const char* tree_hash, const char* prefix);

#endif