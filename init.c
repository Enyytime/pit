#include <stdio.h>
#include <string.h> 
#include <sys/stat.h>
#include "include/hash_object.h"
#include "include/cat_file.h"
#include "include/command_handler.h"


/**
 * @brief Creates a directory, printing an error if it fails.
 *
 * @param path  Directory path to create
 * @return      0 on success, -1 on failure
 */
static int make_dir(const char *path){
    if (mkdir(path, 0755) == -1) {
          perror(path);
          return -1;
    }
      return 0;
}

/**
 * @brief Initializes a new pit repository in the current directory.
 *
 * Creates the .pit/ directory structure and writes the initial HEAD file
 * pointing to refs/heads/main.
 *
 * @return  0 on success, -1 on failure
 */
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
