#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include "include/transport.h"
#include "include/merge.h"
#include "include/http_transport.h"

static int curl_get(const char* url, const char* out) {
    char* a[] = {"curl", "-sf", "-o", (char*)out, (char*)url, NULL};
    return run_cmd(a);
}

static int curl_put(const char* url, const char* file) {
    char data[600];
    snprintf(data, sizeof(data), "@%s", file);
    char* a[] = {"curl", "-sf", "-X", "PUT", "--data-binary", data, (char*)url, NULL};
    return run_cmd(a);
}

static char* slurp(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buf = malloc(n + 1);
    fread(buf, 1, n, f);
    buf[n] = '\0';
    fclose(f);
    return buf;
}

int http_fetch(const char* remote_name, const char* url, const char* branch) {
    char u[1024];
    snprintf(u, sizeof(u), "%s/objects", url);
    if (curl_get(u, ".pit/LIST_TMP") != 0) {
        fprintf(stderr, "error: cannot reach remote (check URL and token)\n");
        return 1;
    }
    char* list = slurp(".pit/LIST_TMP");
    unlink(".pit/LIST_TMP");

    for (char* h = strtok(list, "\n"); h; h = strtok(NULL, "\n")) {
        if (strlen(h) != 40 || object_exists(h)) continue;
        char dir[256], path[300];
        snprintf(dir, sizeof(dir), ".pit/objects/%.2s", h);
        mkdir(dir, 0755);
        snprintf(path, sizeof(path), "%s/%s", dir, h + 2);
        snprintf(u, sizeof(u), "%s/objects/%s", url, h);
        if (curl_get(u, path) != 0) {
            fprintf(stderr, "error: failed to download %s\n", h);
            free(list);
            return 1;
        }
    }
    free(list);

    char dir[256], ref[300];
    mkdir(".pit/refs/remotes", 0755);
    snprintf(dir, sizeof(dir), ".pit/refs/remotes/%s", remote_name);
    mkdir(dir, 0755);
    snprintf(ref, sizeof(ref), "%s/%s", dir, branch);
    snprintf(u, sizeof(u), "%s/refs/%s", url, branch);
    if (curl_get(u, ref) != 0) {
        fprintf(stderr, "error: remote has no branch '%s'\n", branch);
        return 1;
    }
    char hash[41];
    if (read_hash_file(ref, hash) == 0)
        printf("Fetched %s/%s -> %s\n", remote_name, branch, hash);
    return 0;
}

int http_push(const char* url, const char* branch) {
    char local_ref[256], local_hash[41], u[1024];
    snprintf(local_ref, sizeof(local_ref), ".pit/refs/heads/%s", branch);
    if (read_hash_file(local_ref, local_hash) != 0) {
        fprintf(stderr, "error: no commits on '%s', nothing to push\n", branch);
        return 1;
    }

    // what does the remote already have?
    char* have = strdup("");
    snprintf(u, sizeof(u), "%s/objects", url);
    if (curl_get(u, ".pit/LIST_TMP") == 0) {
        free(have);
        have = slurp(".pit/LIST_TMP");
        unlink(".pit/LIST_TMP");
    }

    // fast-forward check
    snprintf(u, sizeof(u), "%s/refs/%s", url, branch);
    if (curl_get(u, ".pit/REMOTE_REF_TMP") == 0) {
        char remote_hash[41];
        int ok = read_hash_file(".pit/REMOTE_REF_TMP", remote_hash);
        unlink(".pit/REMOTE_REF_TMP");
        if (ok == 0) {
            if (strcmp(remote_hash, local_hash) == 0) {
                printf("Everything up-to-date\n");
                free(have);
                return 0;
            }
            if (!object_exists(remote_hash) || !is_ancestor(remote_hash, local_hash)) {
                fprintf(stderr, "error: push rejected, remote has commits you don't have\n");
                fprintf(stderr, "hint: run 'pit pull' first\n");
                free(have);
                return 1;
            }
        }
    }

    // upload objects the remote doesn't have
    DIR* top = opendir(".pit/objects");
    if (!top) { free(have); return 1; }
    struct dirent *d;
    while ((d = readdir(top))) {
        if (strlen(d->d_name) != 2) continue;
        char sub[256];
        snprintf(sub, sizeof(sub), ".pit/objects/%s", d->d_name);
        DIR* sd = opendir(sub);
        if (!sd) continue;
        struct dirent *f;
        while ((f = readdir(sd))) {
            if (f->d_name[0] == '.') continue;
            char hash[64], path[600];
            snprintf(hash, sizeof(hash), "%s%s", d->d_name, f->d_name);
            if (strlen(hash) != 40 || strstr(have, hash)) continue;
            snprintf(path, sizeof(path), "%s/%s", sub, f->d_name);
            snprintf(u, sizeof(u), "%s/objects/%s", url, hash);
            if (curl_put(u, path) != 0) {
                fprintf(stderr, "error: failed to upload %s\n", hash);
                closedir(sd); closedir(top); free(have);
                return 1;
            }
        }
        closedir(sd);
    }
    closedir(top);
    free(have);

    // move the remote branch pointer
    snprintf(u, sizeof(u), "%s/refs/%s", url, branch);
    if (curl_put(u, local_ref) != 0) {
        fprintf(stderr, "error: failed to update remote ref\n");
        return 1;
    }
    printf("Pushed %s (%s)\n", branch, local_hash);
    return 0;
}