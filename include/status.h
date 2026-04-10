#ifndef STATUS_H
#define STATUS_H
#include "file_handler.h"
#include <stdbool.h>

char* get_tree_hash(const char* commit_content);
char* get_commit_tree_content(FileStruct commit, int* tree_size);
bool is_file_changed(char* tree_content, int tree_size, const char* index_file_hash);
void compare_staged_changes();
void compare_not_staged();
void compare_changes();
void pit_status();

#endif