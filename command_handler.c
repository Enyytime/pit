#include <stdio.h>
#include <string.h> 
#include <sys/stat.h>
#include "include/hash_object.h"
#include "include/file_handler.h"
#include "include/cat_file.h"
#include "include/init.h"
#include "include/add.h"
#include "include/write_tree.h"

int parse_command(int argc, char** argv){
    if(argc < 2){
        fprintf(stderr, "usage: pit <command>\n");
        return 1;
    }
    // init
    if(!strcmp(argv[1], "init")){
        return cmd_init();
    }
    // hash
    if(!strcmp(argv[1], "hash-object")){
        cmd_hash_file(argv[2]);
        return 0;
    }
    if(!strcmp(argv[1], "cat-file")){
        cat_file(argv[2]);
        return 0;
    }
    if(!strcmp(argv[1], "add")){
        pit_add(argv[2]);
        return 0;
    }
    if(!strcmp(argv[1], "write-tree")){
        pit_write_tree();
        return 0;
    }
    if(!strcmp(argv[1], "commit-tree")){
      pit_commit_tree(argv[2], argv[3]);
      return 0;
  }
}