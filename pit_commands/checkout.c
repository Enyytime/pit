#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include "include/hash_object.h"
#include "include/file_handler.h"
#include "include/cat_file.h"
#include "include/init.h"
#include "include/add.h"
#include "include/write_tree.h"
#include "include/commit_tree.h"
#include "include/commit.h"
#include "include/log.h"
#include "include/status.h"




void get_mode_hash_filename(char** tree_hash, char** mode, char** hash, char** filename) {
    char* start = *tree_hash;

    *mode = start;
    char* space = strchr(start, ' ');
    *space = '\0';

    *filename = space + 1;
    char* null_pos = strchr(*filename, '\0');
    
    unsigned char* binary_hash = (unsigned char*)(null_pos + 1);
    *hash = malloc(41);  // 40 chars + null terminator
    for(int i = 0; i < 20; i++) {
        sprintf(*hash + (i*2), "%02x", binary_hash[i]);
    }

    (*hash)[40] = '\0';

    *tree_hash = (char*)(binary_hash + 20);
}

void recurse_tree_impl(char* tree_hash, char* current_path);

void recurse_tree(char* tree_hash){
    recurse_tree_impl(tree_hash, "");
}

void recurse_tree_impl(char* tree_hash, char* current_path){
    // printf("DEBUG: Processing tree hash: %s\n", tree_hash);
    int tree_size;
    char* tree_content = cat_file(tree_hash, &tree_size);

    // printf("DEBUG: Tree size: %d bytes\n", tree_size);
    // printf("DEBUG: Tree content (hex): ");
    for(int i = 0; i < tree_size && i < 100; i++) {
        printf("%02x ", (unsigned char)tree_content[i]);
    }
    printf("\n");

    char* start = tree_content;
    char* end = tree_content + tree_size;

    while(start < end) {
        char* mode;
        char* hash;
        char* filename;

        get_mode_hash_filename(&start, &mode, &hash, &filename);
        printf("DEBUG: Found entry - Mode: %s, Filename: %s, Hash: %s\n", mode, filename, hash);

        if (strcmp(mode, "100644") == 0) {
            char full_path[512];
            snprintf(full_path, sizeof(full_path), "%s%s", current_path, filename);
            printf("DEBUG: Writing file: %s\n", full_path);
            int blob_size;
            char* file_content = cat_file(hash, &blob_size);

            FILE* f = fopen(full_path, "wb");
            fwrite(file_content, 1, blob_size, f);
            fclose(f);
            printf("DEBUG: File written successfully\n");

        } else if (strcmp(mode, "40000") == 0) {
            char dir_path[512];
            snprintf(dir_path, sizeof(dir_path), "%s%s", current_path, filename);
            // printf("DEBUG: Found directory: %s, recursing...\n", dir_path);
            // Create directory only if it doesn't exist
            struct stat st;
            if (stat(dir_path, &st) != 0) {
                mkdir(dir_path, 0755);
            }

            char new_path[512];
            // snprintf(new_path, sizeof(new_path), "%s%s/", current_path, filename);
            recurse_tree_impl(hash, new_path);  // Process subtree with new path

        } else {
            printf("DEBUG: Unknown mode: %s for %s\n", mode, filename);
        }
    }
}



void pit_checkout(){
    FileStruct file = init_file_struct(".pit/refs/heads/main");

    char* commit_hash = (char*)read_file_to_string(file);
    commit_hash[strcspn(commit_hash, "\n")] = '\0';
    int content_size = 0;
    char* commit_content = cat_file(commit_hash, &content_size);
    char* tree_hash = get_tree_hash(commit_content);

    recurse_tree(tree_hash);

}
