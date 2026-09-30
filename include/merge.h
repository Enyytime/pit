#ifndef MERGE_H
#define MERGE_H

void pit_merge(const char* branch_name);
int is_ancestor(const char* base_hash, const char* target_hash);
#endif