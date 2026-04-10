#include <stdio.h>
#include "include/file_handler.h"
#include "include/cat_file.h"
#include <string.h>
#include <stdbool.h>


char* get_tree_hash(const char* commit_content){
    char* space  = strchr(commit_content, ' ');
    char* hash = strtok(space + 1, "\n");
    return hash;
}

char* get_commit_tree_content(FileStruct commit){
    char* commit_hash = read_file_to_string(commit);
    char* commit_content = cat_file(commit_hash);
    char* tree_hash = get_tree_hash(commit_content);
    char* tree_content = cat_file(tree_hash);
    return tree_content;
}

bool is_file_changed(const char* tree_content, const char* index_file_hash){
    char* hash_found = strstr(tree_content, index_file_hash);
    if(hash_found){
        return false;
    } else {
        return true;
    }
}

void compare_staged_changes(){
    printf("Changes to be committed:\n");
    FileStruct index = init_file_struct(".pit/index");
    FileStruct commit = init_file_struct(".pit/refs/heads/main");


    char* tree_content = get_commit_tree_content(commit);
    char* index_content = read_file_to_string(index);

    char* index_line = strtok(index_content, "\n");
    while(index_line != NULL){
        char* copy = strdup(index_line);
        strtok(copy, " ");
        char* index_file_hash = strtok(NULL, " ");
        char* index_file_name = strtok(NULL, "\n");

        char* found = strstr(tree_content, index_file_name);
        if (found) {
            if (is_file_changed(tree_content, index_file_name)){
                printf("\tModified: %s\n", index_file_name);
            }
        } else {
            printf("\tNew file: %s\n", index_file_name);
        }

        free(copy);
        index_line = strtok(NULL, "\n");
    }
    fclose(index.file);
    fclose(commit.file);
}

void compare_not_staged();

void compare_changes(){
    compare_staged_changes();
    compare_not_staged();
    compare
}

void pit_status(){

}