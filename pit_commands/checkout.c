#include <stdio.h>
#include <string.h> 
#include <sys/stat.h>
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

char* get_hash_from_tree(char* tree_content, int tree_size, char* path) {
    char* start = tree_content;
    

    while (start < end) {
        
    }
    return NULL;
}


char* get_tree_hash(char* commit_content){
    strtok(commit_content, ' ');
    char* hash = strtok(NULL, "/n");
    return hash;
}

void recurse_tree(char* hash){
        
}

void pit_checkout(){
    FileStruct file = init_file_struct(".pit/refs/heads/main");

    char* commit_hash = read_file_to_string(file);
    int content_size = 0;
    char* commit_content = cat_file(commit_hash, &content_size);
    char tree_hash(commit_content);
    recurse_tree(tree_hash);
    
}