#include <stdio.h>
#include "include/write_tree.h"
#include "include/commit_tree.h"


void pit_commit(const char* commit_message){
    char* tree = read_index();
    char* commit = write_commit_details(tree, commit_message);
}