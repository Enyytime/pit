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
    printf("tree_hash: %s\n", tree_hash);
    char* tree_content = cat_file(tree_hash, tree_size);
    printf("tree_size: %d\n", *tree_size);
    return tree_content;
}

bool is_file_changed(char* tree_content, int tree_size, const char* index_file_hash){
    char* start = tree_content;
    char* end = tree_content + tree_size;

    bool found = false;
    while(start < end){
        char* null_pos = memchr(start, '\0', end - start);

        if (null_pos == NULL) break;

        unsigned char* bin_hash = (unsigned char*)null_pos + 1;
        if ((char*)bin_hash + 20 > end) {
            break;
        }

        char hex[41];
        for(int i = 0; i < 20; i++){
            sprintf(hex + (i * 2), "%02x", bin_hash[i]);
        }
        hex[40] = '\0';

        if (strcmp(hex, index_file_hash) == 0) {
            return false; // hash found, not changed
        }
        start = (char*)bin_hash + 20;
    }
    return true;
}

bool file_found(char* tree_content, int tree_size, char* path){
    char* start = tree_content;
    char* end = tree_content + tree_size;

    char* slash = strchr(path, '/'); // check if if's inside a directory

    while(start < end){
        char* null_pos = memchr(start, '\0', end - start);
        if(null_pos == NULL){
            break;
        }
        char* space = memchr(start, ' ', null_pos - start);
        if(space == NULL) {
            start = (char*)(unsigned char*)null_pos + 20 + 1;
            continue;
        }

        char* mode = start;
        char* filename_in_tree = space + 1;
        if(slash == NULL){
            if (strcmp(filename_in_tree, path) == 0) {
                return true;
            }
        } else {
            // path = "halo/main.c"
            //         ^   ^
            //         |   slash (first '/')
            //         path start
            // str_dir_len = slash - path = 4  →  "halo"
            int str_dir_len = slash - path;

            if(strncmp(filename_in_tree, path, str_dir_len) == 0 && filename_in_tree[str_dir_len] == '\0'){
                unsigned char* bin_hash = (unsigned char*)null_pos + 1;
                char hex[41];
                for(int i = 0; i < 20; i++){
                    sprintf(hex + (i * 2), "%02x", bin_hash[i]);
                }
                hex[40] = '\0';
                int subdir_size;
                char* subdir_content = cat_file(hex, &subdir_size);
                return file_found(subdir_content, subdir_size, slash + 1);
            }
        }
        start = (char*)(unsigned char*)null_pos + 20 + 1;
    }
    return false;
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
        bool found = file_found(tree_content, tree_size, name);

        if (found) {
            if (is_file_changed(tree_content, tree_size, index_file_hash)){
                printf("\tModified: %s\n", index_file_name);
            }
        } else {
            printf("\tNew file: %s\n", index_file_name);
        }

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