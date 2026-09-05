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
#include "include/checkout.h"
#include "include/pack.h"

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
        pit_cat_file(argv[2]);
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

    if(!strcmp(argv[1], "commit")){
        pit_commit(argv[3]);
        return 0;
    }

    if(!strcmp(argv[1], "log")){
        pit_log();
        return 0;
    }

    if(!strcmp(argv[1], "status")){
        pit_status();
        return 0;
    }

    if(!strcmp(argv[1], "checkout")){
        pit_checkout(argv[2]);
        return 0;
    }

    if(!strcmp(argv[1], "read-pack")){
        int count;
        PackObject* objects = read_pack(argv[2], &count);
        if(objects == NULL){
            return 1;
        }
        printf("%d objects\n\n", count);
        for(int i = 0; i < count; i++){
            printf("%-10s size %-6d offset %d\n", get_type_name(objects[i].type),
                   objects[i].size, objects[i].offset);
        }
        free_pack(objects, count);
        return 0;
    }

    return 0;
}
