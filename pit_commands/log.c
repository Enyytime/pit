#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "include/write_tree.h"
#include "include/commit_tree.h"
#include "include/cat_file.h"
#include "include/file_handler.h"

char* get_author(const char* details){
    char* author_line = strstr(details, "author ");
    char* end_line = strchr(author_line, '\n');
    char* author = strndup(author_line, end_line - author_line);
    strtok(author, " ");
    char* name = strtok(NULL, "<");
    char* email = strtok(NULL, ">");
    char* result = malloc(256);
    snprintf(result, 256, "%s <%s>", name, email);
    return result;
}

char* get_date(const char* details){
    char* author_line = strstr(details, "author");
    char* end_line = strchr(author_line, '\n');
    char* author = strndup(author_line, end_line - author_line);
    strtok(author, " ");
    strtok(NULL, " ");
    strtok(NULL, " ");
    char* timestamp_str = strtok(NULL, " ");
    time_t ts = atol(timestamp_str);
    char* date_buf = malloc(64);
    struct tm* t = localtime(&ts);
    strftime(date_buf, 64, "%a %b %d %H:%M:%S %Y", t);
    return date_buf;
}

char* get_commit_message(const char* details){
    char* message_start = strstr(details, "\n\n");
    if (message_start == NULL) return NULL;
    return strdup(message_start + 2);
}

char* get_parent(const char* details){
    char* parent_line = strstr(details, "parent");
    if (parent_line == NULL) return NULL;
    char* end = strchr(parent_line, '\n');
    return strndup(parent_line + 7, end - (parent_line + 7));
}

void pit_log(){
    FILE* head_file = fopen(".pit/HEAD", "r");
    if (!head_file) {
        fprintf(stderr, "not a pit repository\n");
        return;
    }
    char head_line[256];
    fgets(head_line, sizeof(head_line), head_file);
    fclose(head_file);

    char* last_slash = strrchr(head_line, '/');
    if (!last_slash) {
        fprintf(stderr, "detached HEAD not supported\n");
        return;
    }
    char* branch = last_slash + 1;
    branch[strcspn(branch, "\n")] = '\0';

    char ref_path[256];
    snprintf(ref_path, sizeof(ref_path), ".pit/refs/heads/%s", branch);

    FILE* file = fopen(ref_path, "r");
    if (file == NULL) {
        fprintf(stderr, "no commits yet on branch '%s'\n", branch);
        return;
    }

    char commit_hash[41];
    fgets(commit_hash, sizeof(commit_hash), file);
    commit_hash[strcspn(commit_hash, "\n")] = '\0';
    fclose(file);

    while (commit_hash[0] != '\0') {
        char* commit_details = cat_file(commit_hash, NULL);
        char* parent  = get_parent(commit_details);
        char* author  = get_author(commit_details);
        char* date    = get_date(commit_details);
        char* message = get_commit_message(commit_details);

        printf("Commit %s\n", commit_hash);
        printf("Author: %s\n", author);
        printf("Date:   %s\n\n", date);
        printf("    %s", message);

        free(author);
        free(date);
        free(message);
        free(commit_details);

        if (parent == NULL) break;
        strncpy(commit_hash, parent, 41);
        free(parent);
    }
}