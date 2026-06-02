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

static char* get_current_branch_ref(char* ref_path, size_t size) {
    FILE* head = fopen(".pit/HEAD", "r");
    if (!head) {
        return NULL;
    } 

    char line[256];
    fgets(line, sizeof(line), head);
    fclose(head);

    char* last_slash = strrchr(line, '/');
    if (!last_slash) return NULL;
    char* branch = last_slash + 1;

    branch[strcspn(branch, "\n")] = '\0';
    snprintf(ref_path, size, ".pit/refs/heads/%s", branch);
    return ref_path;
    
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


void branch_create(const char* name){
    char ref_path[256];
    get_current_branch_ref(ref_path, sizeof(ref_path));

    FILE* f = fopen(ref_path, "r");
    char hash[41];
    fgets(hash, sizeof(hash), f);
    hash[strcspn(hash, "\n")] = '\0';
    fclose(f);

    // write the current hash to the new branch

    char new_ref[256];
    snprintf(new_ref, sizeof(new_ref), ".pit/refs/heads/%s", name);
    FILE* new_branch = fopen(new_ref, "w");

    fprintf(new_branch, "%s\n", hash);
    fclose(new_branch);
    printf("Created branch '%s'\n", name);

    return;
}

void branch_delete(const char* name) {
    char* current = get_current_branch();

    if (strcmp(current, name) == 0) {
        printf("error: cannot delete branch '%s' — currently on it\n", name);
        free(current);
        return;
    }

    free(current);

    char ref[256];
    snprintf(ref, sizeof(ref), ".pit/refs/heads/%s", name);
    if (remove(ref) != 0) {
        printf("error: branch '%s' not found\n", name);
    } else {
        printf("Deleted branch '%s'\n", name);
    }
    return;
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
        branch_create(name);

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