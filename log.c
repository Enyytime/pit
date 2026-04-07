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

      strtok(author, " "); // skip "author"
      char* name = strtok(NULL, "<"); // get name (stops before email)
      char* email = strtok(NULL, ">"); // get email (between < and >)

      // combine name and email
      char* result = malloc(256);
      snprintf(result, 256, "%s<%s>", name, email);
      return result;
  }

// char* get_date(const char* details){
//     char* author_line = strstr(details, "author");
//     char* end_line = strchr(author_line, '\n');
//     char* author = strndup(author_line, end_line - author_line);

//     strtok(author, " "); // "author"
//     strtok(NULL, " ");   // name
//     strtok(NULL, " ");   // <email>
//     char* timestamp_str = strtok(NULL, " "); // timestamp

//     time_t ts = atol(timestamp_str);
//     char* date_buf = malloc(64);
//     struct tm* t = localtime(&ts);
//     strftime(date_buf, 64, "%a %b %d %H:%M:%S %Y", t);
//     return date_buf;
// }

char* get_commit_message(const char* details){
    char* message_start = strstr(details,"\n\n");

    if(message_start == NULL){
        return NULL;
    }

    return strdup(message_start + 2);
}

char* get_parent(const char* details){
    char* parent_line = strstr(details, "parent");
    if(parent_line == NULL){
        return NULL;
    }

    char* end = strchr(parent_line, '\n');
    char* parent_hash = strndup(parent_line + 7, end - (parent_line + 7));

    return parent_hash;
}


char* create_log_message(const char* commit_hash){
    char* commit_details = cat_file(commit_hash);
    return commit_details;
}


void pit_log(){
    FILE* file = fopen(".pit/refs/heads/main", "r");
    if(file == NULL){
        fprintf(stderr, "no commits yet\n");
        return;
    }
    char commit_hash[41];
    fgets(commit_hash, sizeof(commit_hash), file);

    commit_hash[strcspn(commit_hash, "\n")] = '\0';
    fclose(file);

    while(commit_hash[0] != '\0'){
        char* commit_details = create_log_message(commit_hash);
        char* parent = get_parent(commit_details);

        char* author = get_author(commit_details);
        // char* date = get_date(commit_details);
        char* message = get_commit_message(commit_details);

        printf("Author: %s\n", author);
        // printf("Date:   %s\n", date);
        printf("    %s", message);

        if(parent == NULL) break;
        strncpy(commit_hash, parent, 41);
    }

}