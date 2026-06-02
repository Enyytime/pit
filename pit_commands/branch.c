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

char* get_current_branch() {
    FileStruct f = init_file_struct(".pit/HEAD");
    char* head_content = (char*)read_file_to_string(f);
    fclose(f.file);  // close it here

    // parse the branch name
    char* last_slash = strrchr(head_content, '/');
    char* branch_name = last_slash + 1;
    branch_name[strcspn(branch_name, "\n")] = '\0';

    return branch_name;  // caller must free head_content when done
}

void branch_list() {
    DIR* dir = opendir(".pit/refs/heads");
    if (dir == NULL) {
        perror("opendir failed");
        return;
    }

    char* current_dir = get_current_branch();
    struct dirent* entry;

    while((entry = readdir(dir)) != NULL) {
        // skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        if(strcmp(entry->d_name, current_dir) == 0) {
            printf("*%s\n", entry->d_name);
        } else {
            printf("%s\n", entry->d_name);
        }
        
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
        branch_list();

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