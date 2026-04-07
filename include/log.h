#ifndef LOG_H
#define LOG_H

char* get_author(const char* details);
char* get_date(const char* details);
char* get_commit_message(const char* details);
char* get_parent(const char* details);
char* create_log_message(const char* commit_hash);
void pit_log();

#endif