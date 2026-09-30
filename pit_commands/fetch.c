#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/remote.h"
#include "include/branch.h"
#include "include/transport.h"
#include "include/fetch.h"
#include "include/http_transport.h"

static int fetch_branch(const char* remote_name, const char* url, const char* branch) {
    // 1. download objects we don't have
    char src_objects[512];
    snprintf(src_objects, sizeof(src_objects), "%s/.pit/objects/", url);
    char* rsync_args[] = {"rsync", "-a", "--ignore-existing",
                          src_objects, ".pit/objects/", NULL};
    if (run_cmd(rsync_args) != 0) {
        fprintf(stderr, "error: failed to download objects\n");
        return 1;
    }

    // 2. save remote branch pointer as .pit/refs/remotes/<remote>/<branch>
    char dir[256];
    snprintf(dir, sizeof(dir), ".pit/refs/remotes/%s", remote_name);
    char* mkdir_args[] = {"mkdir", "-p", dir, NULL};
    run_cmd(mkdir_args);

    char remote_ref[512], local_ref[256];
    snprintf(remote_ref, sizeof(remote_ref), "%s/.pit/refs/heads/%s", url, branch);
    snprintf(local_ref, sizeof(local_ref), "%s/%s", dir, branch);

    char* scp_args[] = {"scp", "-q", remote_ref, local_ref, NULL};
    if (run_cmd(scp_args) != 0) {
        fprintf(stderr, "error: remote has no branch '%s'\n", branch);
        return 1;
    }

    char hash[41];
    if (read_hash_file(local_ref, hash) == 0)
        printf("Fetched %s/%s -> %s\n", remote_name, branch, hash);
    return 0;
}

void pit_fetch(const char* remote_name) {
    char* url = remote_get_url(remote_name);
    if (!url) {
        fprintf(stderr, "error: remote '%s' not found\n", remote_name);
        return;
    }
    char* branch = get_current_branch();
    if (strncmp(url, "http", 4) == 0)
        http_fetch(remote_name, url, branch);
    else
        fetch_branch(remote_name, url, branch);
    free(branch);
    free(url);
}