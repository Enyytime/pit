#include <stdio.h>
#include "include/file_handler.h"
#include "include/cat_file.h"
#include "include/hash_object.h"
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>


char* get_tree_hash(const char* commit_content){
    char* space  = strchr(commit_content, ' ');
    char* hash = strtok(space + 1, "\n");
    return hash;
}

char* get_commit_tree_content(FileStruct commit, int* tree_size){
    char* commit_hash = (char*)read_file_to_string(commit);
    commit_hash[commit.filesize] = '\0';
    commit_hash[strcspn(commit_hash, "\n")] = '\0';
    
    char* commit_content = cat_file(commit_hash, NULL);
    char* tree_hash = get_tree_hash(commit_content);
    
    char* tree_content = cat_file(tree_hash, tree_size);
    
    return tree_content;
}

char* get_hash_from_tree(char* tree_content, int tree_size, char* path) {
    char* start = tree_content;
    char* end = tree_content + tree_size;
    char* slash = strchr(path, '/');

    while (start < end) {
        char* null_pos = memchr(start, '\0', end - start);
        if (null_pos == NULL) break;
        char* space = memchr(start, ' ', null_pos - start);
        if (space == NULL) { start = (char*)null_pos + 21; continue; }

        char* filename_in_tree = space + 1;
        unsigned char* bin_hash = (unsigned char*)null_pos + 1;

        char hex[41];
        for (int i = 0; i < 20; i++) sprintf(hex + (i*2), "%02x", bin_hash[i]);
        hex[40] = '\0';

        if (slash == NULL) {
            if (strcmp(filename_in_tree, path) == 0) return strdup(hex);
        } else {
            
            // path = "halo/main.c"
            //         ^   ^
            //         |   slash (first '/')
            //         path start
            // str_dir_len = slash - path = 4  →  "halo"
            int dir_len = slash - path;
            if (strncmp(filename_in_tree, path, dir_len) == 0 && filename_in_tree[dir_len] == '\0') {
                int sub_size;
                char* sub_content = cat_file(hex, &sub_size);
                return get_hash_from_tree(sub_content, sub_size, slash + 1);
            }
        }
        start = (char*)null_pos + 21;
    }
    return NULL;
}

void compare_staged_changes(){
    printf("Changes to be committed:\n\n");
    FileStruct index = init_file_struct(".pit/index");
    FileStruct commit = init_file_struct(".pit/refs/heads/main");

    int tree_size = 0;
    char* tree_content = get_commit_tree_content(commit, &tree_size);
    char* index_content = (char*)read_file_to_string(index);

    char* index_line = strtok(index_content, "\n");
    while(index_line != NULL){

        char mode[16], hash[41], filename[256];
        sscanf(index_line, "%s %s %s", mode, hash, filename);
        char* index_file_hash = hash;
        char* index_file_name = filename;

        char* name = index_file_name;
        if (strncmp(name, "./", 2) == 0) name += 2;

        // walk tree entries to find filename
        char* p = tree_content;
        char* end = tree_content + tree_size;
        char* tree_hash = get_hash_from_tree(tree_content, tree_size, name);
        if (tree_hash == NULL) {
            printf("\tNew file: %s\n", index_file_name);
        } else if (strcmp(tree_hash, index_file_hash) != 0) {
            printf("\tModified: %s\n", index_file_name);
        }
        free(tree_hash);

        index_line = strtok(NULL, "\n");
    }
    printf("\n");
    fclose(index.file);
    fclose(commit.file);
}

void compare_not_staged(){
    printf("Changes not staged for commit:\n\n");
    FileStruct index = init_file_struct(".pit/index");
    char* index_content = (char*)read_file_to_string(index);

    char* index_line = strtok(index_content, "\n");

    while(index_line != NULL){
        

        char mode[16], hash[41], filename[256];
        sscanf(index_line, "%s %s %s", mode, hash, filename);

        char* index_file_hash = hash;
        char* index_file_name = filename;
        char* name = index_file_name;
        if (strncmp(name, "./", 2) == 0) name += 2;

        FileStruct f = init_file_struct(index_file_name);
        unsigned char* content = read_file_to_string(f);

        char* current_hash = compute_hash("blob", content, f.filesize);
        fclose(f.file);
        free(content);
        
        if(strcmp(index_file_hash, current_hash) != 0){
            printf("\tModified: %s\n", index_file_name);
        } 

        free(current_hash);
        index_line = strtok(NULL, "\n");
    }
    printf("\n");
    fclose(index.file);
}

void compare_changes(){
    printf("On branch main\n\n");
    compare_staged_changes();
    compare_not_staged();
}

void pit_status(){
    compare_changes();
}