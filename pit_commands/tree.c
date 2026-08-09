#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include "include/tree.h"
#include "include/refs.h"
#include "include/cat_file.h"


/**
 * @brief Converts 20 raw hash bytes into a 40-character hex string.
 *
 * @param raw  The 20 bytes stored in the tree entry
 * @param out  Buffer of at least 41 bytes
 */
static void hash_to_hex(const unsigned char* raw, char* out) {
    for (int i = 0; i < 20; i++) {
        sprintf(out + (i * 2), "%02x", raw[i]);
    }
    out[HASH_HEX_LEN] = '\0';
}


/**
 * @brief Reads a tree object and returns its entries.
 *
 * Each entry is "<mode> <name>\0<20 raw hash bytes>" with no separator
 * between entries. The hash is binary, not hex, so it can contain
 * spaces and NUL bytes — the parser advances exactly 20 bytes rather
 * than scanning for a delimiter.
 *
 * Covers one directory only. Entries with is_dir set point to another
 * tree object that must be parsed with a further call.
 *
 * @param tree_hash  40-char hex hash of the tree object
 * @param count      Output: number of entries found
 * @return           Heap-allocated array of TreeItem, or NULL on
 *                   failure — caller must free
 */
TreeItem* parse_tree(const char* tree_hash, int* count) {
    *count = 0;
    
    int size;

    unsigned char* body = (unsigned char*)cat_file(tree_hash, &size);
    if (body == NULL) {
        return NULL;
    }

    int capacity = 16;
    int length = 0;

    TreeItem* items = (TreeItem*)malloc(sizeof(TreeItem) * capacity);

    int pos = 0;

    while(pos < size) {
        // Extract the mode
        char* space = strchr((char*)body + pos, ' ');
        if (space == NULL) {
            break;
        }
        int mode_length = space - ((char*)body + pos);

        if(length >= capacity){
            capacity *= 2;
            items = (TreeItem*)realloc(items, sizeof(TreeItem) * capacity);
        }

        memcpy(items[length].mode, body + pos, mode_length);
        items[length].mode[mode_length] = '\0';

        pos += mode_length + 1; // move the position

        // Extract the name, since the name has a NULL, we can just use strlen
        int name_length = strlen((char*)body + pos);

        if(length >= capacity){
            capacity *= 2;
            items = (TreeItem*)realloc(items, sizeof(TreeItem) * capacity);
        }

        memcpy(items[length].name, body + pos, name_length);
        items[length].name[name_length] = '\0';

        pos += name_length + 1;   // past the name and the NUL

        hash_to_hex(body + pos, items[length].hash);
        pos += 20;

        items[length].is_dir = !strcmp(items[length].mode, "40000");

        length++;
    }

    free(body);
    *count = length;
    return items;
}


/**
 * @brief Recursively visits every file beneath a tree.
 *
 * Descends into subdirectories by calling parse_tree again on each
 * one, building up the path as it goes. Callers that want to skip
 * identical subtrees should use parse_tree directly instead.
 *
 * @param tree_hash  40-char hex hash of the tree to walk
 * @param prefix     Path prepended to each entry; "" at the root
 * @param visit      Called once per file, never for directories
 * @param userdata   Passed through to visit, untouched
 */
void walk_tree(const char* tree_hash, const char* prefix){
    int count;
    TreeItem* items = parse_tree(tree_hash, &count);
    if(items == NULL){
        return;
    }

    for(int i = 0; i < count; i++){
        char path[1024];
        if(strlen(prefix) == 0){
            snprintf(path, sizeof(path), "%s", items[i].name);
        } else {
            snprintf(path, sizeof(path), "%s/%s", prefix, items[i].name);
        }

        if(items[i].is_dir){
            walk_tree(items[i].hash, path);
        } else {
            printf("%s %s\t%s\n", items[i].mode, items[i].hash, path);
        }
    }

    free(items);
}