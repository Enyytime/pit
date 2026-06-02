#define _DEFAULT_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "include/hash_object.h"
#include "include/entry.h"
#include <dirent.h>
#include <stdbool.h>
#include <time.h>

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

static char* get_current_branch_ref(char* ref_path, size_t size) {
    FILE* head = fopen(".pit/HEAD", "r");
    if (!head) return NULL;
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

char* write_commit_details(const char* tree_hash, const char* commit_message) {
    FILE* config = fopen(".pit/config", "r");
    char* name = get_name(config);
    rewind(config);
    char* email = get_email(config);
    fclose(config);

    time_t timestamp = time(NULL);
    char* content;

    char ref_path[256];
    get_current_branch_ref(ref_path, sizeof(ref_path));

    char parent[41];
    FILE* head = fopen(ref_path, "r");

    if (head == NULL) {
        content = first_commit(tree_hash, commit_message, name, email, timestamp);
    } else {
        fgets(parent, sizeof(parent), head);
        parent[strcspn(parent, "\n")] = '\0';
        fclose(head);
        content = regular_commit(tree_hash, commit_message, name, email, timestamp, parent);
    }

    char* hex = store_object("commit", (unsigned char*)content, strlen(content));
    free(content);

    FILE* ref = fopen(ref_path, "w");
    fprintf(ref, "%s\n", hex);
    fclose(ref);

    free(name);
    free(email);
    return hex;
}

void pit_commit_tree(const char* tree_hash, const char* commit_message) {
    char* commit = write_commit_details(tree_hash, commit_message);
    printf("%s\n", commit);
    free(commit);
}