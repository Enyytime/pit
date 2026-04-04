#include <stdio.h>
#include <string.h> 
#include <sys/stat.h>
#include "include/hash_object.h"

static int make_dir(const char *path){
    if (mkdir(path, 0755) == -1) {
          perror(path);
          return -1;
    }
      return 0;
}

int cmd_init(){
    make_dir(".pit");
    make_dir(".pit/objects");
    make_dir(".pit/objects/pack");
    make_dir(".pit/objects/info");
    make_dir(".pit/refs");
    make_dir(".pit/refs/heads");
    make_dir(".pit/refs/tags");

    FILE *f = fopen(".pit/HEAD", "w");
    if (!f) { perror(".pit/HEAD"); return -1; }
    fprintf(f, "ref: refs/heads/main\n");
    fclose(f);

    printf("Initialized empty pit repository in .pit/\n");

    return 0; // success
}



int main(int argc, char** argv){

    
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
        hash_file(argv[2]);
    }
    if(!strcmp(argv[1], "cat-file")){
        cat_file(argv[2]);
    }
    return 0;
}