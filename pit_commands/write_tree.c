#define _DEFAULT_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "include/hash_object.h"
#include "include/entry.h"
#include "include/dequeue.h"
#include <dirent.h>
#include <stdbool.h>


int length = 0;
int capacity = 10;


typedef struct directory_depth {
    int depth;
    char* parent;
    char* prefix;
    char* full_path;
    char* hash;  // set after processing
} Dir_depth;

int compare_depth(const void* a, const void* b) {
    Dir_depth* da = (Dir_depth*)a;
    Dir_depth* db = (Dir_depth*)b;
    return db->depth - da->depth; 
}

char* build_tree_entry(const char* mode, const char* hash, const char* filename, int* line_length){
    unsigned char bin[20];
    for(int i = 0; i < 20; i++){
        sscanf(hash + (i * 2), "%2hhx", &bin[i]);
    }
    
    int entry_len = strlen(mode) + 1 + strlen(filename) + 1 + 20;
    char* return_string = (char*)malloc(sizeof(char) * entry_len);
    int offset = 0;
    memcpy(return_string + offset, mode, strlen(mode));
    offset += strlen(mode);
    return_string[offset++] = ' ';
    memcpy(return_string + offset, filename, strlen(filename));
    offset += strlen(filename);
    return_string[offset++] = '\0';
    memcpy(return_string + offset, bin, 20);
    
    *line_length = entry_len;
    return return_string;
}

void* append_tree(char** tree, const char* tree_line, int line_length){
    for(int i = 0; i < line_length; i++){
        if(length >= capacity){
            capacity *= 2;
            *tree = realloc(*tree, sizeof(char) * capacity);
        }
        (*tree)[length] = tree_line[i];
        length++;
    }
}

char** split_string_on_slash(char* str, size_t* numFields) {
    *numFields = 0;
    int capacity = 10;
    char** fields = malloc(sizeof(char*) * (capacity + 1));

    fields[(*numFields)++] = str;

    for (char* p = str; *p != '\0'; p++) {
        if (*p == '/') {
            *p = '\0';
            if ((int)(*numFields) >= capacity) {
                capacity *= 2;
                fields = realloc(fields, sizeof(char*) * (capacity + 1));
            }
            fields[(*numFields)++] = p + 1;
        }
    }

    // remove last element (filename)
    (*numFields)--;
    fields[*numFields] = NULL;
    return fields;
}

int get_depth(const char* filename) {
    int count = 0;
    int len = strlen(filename);
    for(int i = 0; i < len; i++){
        char c = filename[i];
        if(c == '/'){
            count++;
        }
    }
    return count;
}

Dir_depth* put_index_to_dirs(FILE* file, int* dir_length_out){
    int dir_capacity = 10;
    int dir_length = 0;

    Dir_depth* dirs = (Dir_depth*)malloc(sizeof(Dir_depth) * dir_capacity);

    char line[256];
    while(fgets(line, sizeof(line), file) != NULL){
        strtok(line, " ");
        strtok(NULL, " ");
        char* filename = strtok(NULL, "\n");
        int depth = get_depth(filename);
        
        if(depth == 0){
            continue;
        }

        size_t numFields;
        char** parts = split_string_on_slash(filename, &numFields);

        for (int i = 0; i < (int)numFields; i++) {
            char full_path[256] = "";

            for (int j = 0; j <= i; j++) {

                if (j > 0){
                    strcat(full_path, "/");
                }

                strcat(full_path, parts[j]);
            }

            bool found = false;
            for (int k = 0; k < dir_length; k++) {

                if (strcmp(dirs[k].prefix, full_path) == 0) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                if (dir_length >= dir_capacity) {
                    dir_capacity *= 2;
                    dirs = realloc(dirs, sizeof(Dir_depth) * dir_capacity);
                }
                dirs[dir_length].full_path = strdup(full_path);
                dirs[dir_length].depth = i + 1;
                dirs[dir_length].parent = (i == 0) ? NULL : strdup(parts[i - 1]);
                dirs[dir_length].prefix = strdup(parts[i]);
                dir_length++;
            }
        }
        free(parts);
    }
    *dir_length_out = dir_length;
    return dirs;
}

char* read_index(){
    FILE* file = fopen(".pit/index", "r");

    char line[256]; 

    if(file == NULL){
        return NULL; 
    }
    int dir_length;

    Dir_depth* dirs = put_index_to_dirs(file, &dir_length);
    qsort(dirs, dir_length, sizeof(Dir_depth), compare_depth);

    for(int i = 0; i < dir_length; i++){
        length = 0;
        capacity = 10;
        char* subtree = (char*)malloc(sizeof(char) * capacity);

        // build entry in current subdirectories
        while(fgets(line, sizeof(line), file) != NULL){
            char *mode = strtok(line, " ");
            char *hash = strtok(NULL, " ");
            char *filename = strtok(NULL, "\n");

            int full_path_len = strlen(dirs[i].full_path);
            // src/
            // if the filename is not the same with the one we're checking
            // skip it
            if(strncmp(filename, dirs[i].full_path, full_path_len)){
                continue;
            }
            if(filename[full_path_len] != '/') {
                continue;
            }

            char* direct_child = filename + full_path_len + 1;
            // check if there is another subdirectory or not
            if(strchr(direct_child, '/') != NULL) {
                continue; 
            }

            int line_length;
            char* line_entry = build_tree_entry(mode, hash, filename, &line_length);
            append_tree(&subtree, line_entry, line_length);
        }

        for(int j = 0; j < dir_length; j++){
            // skip if there's no hash yet
            if(dirs[j].hash == NULL){
                continue;
            }
            // skip 
            if(dirs[j].parent == NULL){
                continue;
            }
            // if we're checking it self, skip
            if(!strcmp(dirs[j].parent, dirs[i].prefix)) {
                continue;
            }

            if(dirs[j].depth != dirs[i].depth + 1){
                continue;
            }

            int line_length;
            char* entry = build_tree_entry("40000", dirs[j].hash, dirs[j].prefix, &line_length);
            append_tree(&subtree, entry, line_length);
        }

        dirs[i].hash = store_object("tree", (unsigned char*)subtree, length);
        free(subtree);
    }

    length = 0;
    capacity = 10;
    char* tree = (char*)malloc(sizeof(char) * capacity);

    
    while(fgets(line, sizeof(line), file) != NULL){
        char *mode = strtok(line, " ");
        char *hash = strtok(NULL, " ");
        char *filename = strtok(NULL, "\n");
        if (strchr(filename, '/') != NULL) continue;
        int line_length;
        char* line_entry = build_tree_entry(mode, hash, filename, &line_length);
        append_tree(&tree, line_entry, line_length);
        free(line_entry);
    }

    for(int i = 0; i < dir_length; i++){
        if(dirs[i].depth != 1){
            continue;
        }
        int line_length;
        char* line_entry = build_tree_entry("40000", dirs[i].hash, dirs[i].prefix, &line_length);
        append_tree(&tree, line_entry, line_length);
    }

    char* hex = store_object("tree", (unsigned char*)tree, length);

    length = 0;
    capacity = 10;

    fclose(file);
    return hex;
}

void pit_write_tree(){
    char* tree = read_index();
    printf("%s\n", tree);
}