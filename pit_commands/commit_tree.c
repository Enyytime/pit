#define _DEFAULT_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "include/hash_object.h"
#include "include/entry.h"
#include <dirent.h>
#include <stdbool.h>
#include <time.h>
#include "include/refs.h"


char* get_name(FILE* file){
    char buffer[256];
    while(fgets(buffer, sizeof(buffer), file) != NULL){
        if(strncmp(buffer, "name=", 5) == 0){
            char* val = strdup(buffer + 5);
            val[strcspn(val, "\n")] = '\0';
            return val;
        }
    }
    return NULL;
}

char* get_email(FILE* file){
    char buffer[256];
    while(fgets(buffer, sizeof(buffer), file) != NULL){
        if(strncmp(buffer, "email=", 6) == 0){
            char* val = strdup(buffer + 6);
            val[strcspn(val, "\n")] = '\0';
            return val;
        }
    }
    return NULL;
}

char* first_commit(const char* tree_hash, const char* commit_message, const char* name, const char* email, time_t timestamp) {
    char* content = malloc(1024);
    int len = 0;
    len += snprintf(content + len, 1024 - len, "tree %s\n", tree_hash);
    len += snprintf(content + len, 1024 - len, "author %s <%s> %ld +0000\n", name, email, timestamp);
    len += snprintf(content + len, 1024 - len, "committer %s <%s> %ld +0000\n", name, email, timestamp);
    len += snprintf(content + len, 1024 - len, "\n%s\n", commit_message);
    return content;
}

char* regular_commit(const char* tree_hash, const char* commit_message, const char* name, const char* email, time_t timestamp, const char* parent){
    char* content = malloc(1024);
    int len = 0;
    len += snprintf(content + len, 1024 - len, "tree %s\n", tree_hash);
    len += snprintf(content + len, 1024 - len, "parent %s\n", parent);
    len += snprintf(content + len, 1024 - len, "author %s <%s> %ld +0000\n", name, email, timestamp);
    len += snprintf(content + len, 1024 - len, "committer %s <%s> %ld +0000\n", name, email, timestamp);
    len += snprintf(content + len, 1024 - len, "\n%s\n", commit_message);
    return content;
}

char* write_commit_details(const char* tree_hash, const char* commit_message) {
    FILE* config = fopen(".pit/config", "r");
    if (config == NULL) {
        fprintf(stderr, "cannot read .pit/config\n");
        return NULL;
    }
    char* name = get_name(config);
    rewind(config);
    char* email = get_email(config);
    fclose(config);

    time_t timestamp = time(NULL);

    char* branch = current_branch();
    if (branch == NULL) {
        fprintf(stderr, "HEAD is not on a branch\n");
        free(name);
        free(email);
        return NULL;
    }

    /* Read the branch tip before building the commit. This same value
       is passed to update_ref below as the expected old hash, so the
       commit is only applied if the branch has not moved since. */
    char* parent = read_ref(branch);

    char* content;
    if (parent == NULL) {
        content = first_commit(tree_hash, commit_message,
                               name, email, timestamp);
    } else {
        content = regular_commit(tree_hash, commit_message,
                                 name, email, timestamp, parent);
    }

    char* hex = store_object("commit", (unsigned char*)content,
                             strlen(content));
    free(content);

    RefResult r = update_ref(branch, hex, parent);

    free(name);
    free(email);
    free(branch);
    free(parent);

    if (r != REF_OK) {
        const char* why =
            (r == REF_STALE)  ? "branch moved since the commit was built" :
            (r == REF_LOCKED) ? "another process holds the branch lock"   :
                                "I/O error writing the reference";
        fprintf(stderr, "commit not applied: %s\n", why);
        free(hex);
        return NULL;
    }

    return hex;
}

void pit_commit_tree(const char* tree_hash, const char* commit_message) {
    char* commit = write_commit_details(tree_hash, commit_message);
    if (commit == NULL) return;
    printf("%s\n", commit);
    free(commit);
}