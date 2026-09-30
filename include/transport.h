#ifndef TRANSPORT_H
#define TRANSPORT_H
int run_cmd(char* const argv[]);
int object_exists(const char* hash);
int read_hash_file(const char* path, char* out);
#endif