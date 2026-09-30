#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "include/transport.h"

// run a command without going through the shell (no injection issues)
int run_cmd(char* const argv[]) {
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        execvp(argv[0], argv);
        _exit(127);
    }
    int status;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

int object_exists(const char* hash) {
    char path[256];
    snprintf(path, sizeof(path), ".pit/objects/%.2s/%s", hash, hash + 2);
    return access(path, F_OK) == 0;
}

// out must be at least 41 bytes. returns 0 on success
int read_hash_file(const char* path, char* out) {
    FILE* f = fopen(path, "r");
    if (!f) return -1;
    if (!fgets(out, 41, f)) { fclose(f); return -1; }
    fclose(f);
    out[strcspn(out, "\n")] = '\0';
    return strlen(out) == 40 ? 0 : -1;
}