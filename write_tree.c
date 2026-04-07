#define _DEFAULT_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "include/hash_object.h"
#include "include/entry.h"
#include <dirent.h>
#include <stdbool.h>


int length = 0;
int capacity = 10;


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

char* read_index(){
    FILE* file = fopen(".pit/index", "r");

    char line[256]; 

    if(file == NULL){
        return NULL; 
    }

    char* tree = (char*)malloc(sizeof(char) * capacity);

    while(fgets(line, sizeof(line), file) != NULL){
        char *mode = strtok(line, " ");
        char *hash = strtok(NULL, " ");
        char *filename = strtok(NULL, "\n");
        int line_length;
        char* line_entry = build_tree_entry(mode, hash, filename, &line_length);
        append_tree(&tree, line_entry, line_length);
    }


    char* hex = store_object("tree", (unsigned char*)tree, length);

    length = 0;
    capacity = 10;

    return hex;
}

void pit_write_tree(){
    char* tree = read_index();
    printf("%s\n", tree);
}