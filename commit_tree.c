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
            return strdup(buffer + 5);
        }
    }
}

char* get_email(FILE* file){
    char buffer[256];
    while(fgets(buffer, sizeof(buffer), file) != NULL){
        if(strncmp(buffer, "email=", 6) == 0){
            return strdup(buffer + 6);
        }
    }
}

char* first_commit(const char* tree_hash, const char* commit_message, const char* name, const char* email, time_t timestamp) {
    char* content = malloc(1024);

    int len = 0;
    len += snprintf(content + len, 1024 - len, "tree %s\n", tree_hash);
    len += snprintf(content + len, sizeof(content) - len, "author %s <%s> %ld +0000\n", name, email, timestamp);
    len += snprintf(content + len, sizeof(content) - len, "committer %s <%s> %ld +0000\n", name, email, timestamp);
    len += snprintf(content + len, sizeof(content) - len, "\n%s\n", commit_message);


    return content;
}

char* regular_commit(const char* tree_hash, const char* commit_message, const char* name, const char* email, time_t timestamp, const char* parent){
    char* content = malloc(1024);

    int len = 0;
    len += snprintf(content + len, 1024 - len, "tree %s\n", tree_hash);
    len += snprintf(content + len, sizeof(content) - len, "parent %s\n", parent);
    len += snprintf(content + len, sizeof(content) - len, "author %s <%s> %ld +0000\n", name, email, timestamp);
    len += snprintf(content + len, sizeof(content) - len, "committer %s <%s> %ld +0000\n", name, email, timestamp);
    len += snprintf(content + len, sizeof(content) - len, "\n%s\n", commit_message);

    return content;
}


void write_commit_details(const char* tree_hash, const char* commit_message) {

    FILE* config = fopen(".pit/config", "r");
    char* name = get_name(config);
    rewind(config);
    char* email = get_email(config);

    time_t timestamp = time(NULL);
    char* content;

    char parent[41];
    FILE* head = fopen(".pit/refs/heads/main", "r");

    if(head == NULL){
        content = first_commit(tree_hash, commit_message, name, email, timestamp);
    } else {
        fgets(parent, sizeof(parent), head);
        content = regular_commit(tree_hash, commit_message, name, email, timestamp, parent);
        fclose(head);
    }

    char* hex = store_object("commit", (unsigned char*)content, strlen(content));
    printf("%s\n", hex);      

    FILE* ref = fopen(".pit/refs/heads/main", "w");
    fprintf(ref, "%s\n", hex);
    fclose(ref);

    return ;
}



void pit_commit_tree(const char* tree_hash, const char* commit_message) {
    write_commit_details(tree_hash, commit_message);
}