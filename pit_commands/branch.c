#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
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

char* get_current_dir() {
    FileStruct f = init_file_struct(".pit/HEAD");
    
    char* head_content = (char*)read_file_to_string(f);

    
}

void branch_list() {
    DIR* dir = opendir(".pit/refs/heads");
    if (dir == NULL) {
        perror("opendir failed");
        return;
    }

    char* current_dir;
    struct dirent* entry;

    while((entry = readdir(dir)) != NULL) {
        // skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        printf("%s\n", entry->d_name);
    }

}



void pit_branch(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: pit branch <create|list|delete> [name]\n");
        return;
    }

    char* subcmd = argv[1];

    if (strcmp(subcmd, "create") == 0) {
        if (argc < 3) {
            printf("Usage: pit branch create <name>\n");
            return;
        }
        char* name = argv[2];
        // 1. read current HEAD commit hash from refs/heads/main (or current branch)
        // 2. write hash to .pit/refs/heads/<name>

    } else if (strcmp(subcmd, "list") == 0) {
        // 1. opendir(".pit/refs/heads/")
        // 2. readdir() each entry
        // 3. read HEAD to know current branch
        // 4. print with * on current

    } else if (strcmp(subcmd, "delete") == 0) {
        if (argc < 3) {
            printf("Usage: pit branch delete <name>\n");
            return;
        }
        char* name = argv[2];
        // 1. check name != current branch (read HEAD)
        // 2. remove(".pit/refs/heads/<name>")
    }
}