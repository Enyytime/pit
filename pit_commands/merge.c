#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "include/cat_file.h"
#include "include/file_handler.h"
#include "include/checkout.h"

// read commit hash from a branch ref file
static char* read_ref(const char* branch) {
    char ref_path[256];
    snprintf(ref_path, sizeof(ref_path), ".pit/refs/heads/%s", branch);

    FILE* f = fopen(ref_path, "r");
    if (!f) return NULL;

    char* hash = malloc(41);
    fgets(hash, 41, f);
    hash[strcspn(hash, "\n")] = '\0';
    fclose(f);
    return hash;
}

// get parent hash from a commit, returns NULL if first commit
static char* get_parent(const char* commit_hash) {
    int size;
    char* content = cat_file(commit_hash, &size);
    char* parent_line = strstr(content, "parent ");
    if (!parent_line) {
        free(content);
        return NULL;
    }
    char* end = strchr(parent_line, '\n');
    char* parent = strndup(parent_line + 7, end - (parent_line + 7));
    free(content);
    return parent;
}

// walk target's history, check if base_hash appears in it
static int is_ancestor(const char* base_hash, const char* target_hash) {
    char* current = strdup(target_hash);

    while (current != NULL) {
        if (strcmp(current, base_hash) == 0) {
            free(current);
            return 1;  // found it — base is ancestor of target
        }
        char* parent = get_parent(current);
        free(current);
        current = parent;
    }
    return 0;
}

void pit_merge(const char* branch_name) {
    // get current branch name
    FILE* head_file = fopen(".pit/HEAD", "r");
    if (!head_file) {
        fprintf(stderr, "not a pit repository\n");
        return;
    }
    char head_line[256];
    fgets(head_line, sizeof(head_line), head_file);
    fclose(head_file);

    char* last_slash = strrchr(head_line, '/');
    char* current_branch = last_slash + 1;
    current_branch[strcspn(current_branch, "\n")] = '\0';

    // can't merge branch into itself
    if (strcmp(current_branch, branch_name) == 0) {
        printf("error: cannot merge branch into itself\n");
        return;
    }

    char* current_hash = read_ref(current_branch);
    char* target_hash  = read_ref(branch_name);

    if (!target_hash) {
        printf("error: branch '%s' not found\n", branch_name);
        free(current_hash);
        return;
    }

    // already up to date
    if (strcmp(current_hash, target_hash) == 0) {
        printf("Already up to date.\n");
        free(current_hash);
        free(target_hash);
        return;
    }

    // check fast-forward: is current an ancestor of target?
    if (is_ancestor(current_hash, target_hash)) {
        // fast-forward — update current branch ref to target hash
        char ref_path[256];
        snprintf(ref_path, sizeof(ref_path), ".pit/refs/heads/%s", current_branch);

        FILE* ref = fopen(ref_path, "w");
        fprintf(ref, "%s\n", target_hash);
        fclose(ref);

        // update working tree
        pit_checkout(current_branch);

        printf("Fast-forward merge: %s -> %s\n", current_hash, target_hash);
    } else {
        printf("error: histories have diverged — automatic merge not supported yet\n");
        printf("hint: rebase your branch onto %s first\n", current_branch);
    }

    free(current_hash);
    free(target_hash);
}