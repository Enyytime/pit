#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "include/remote.h"
#include "include/merge.h"
#include "include/branch.h"
#include "include/transport.h"
#include "include/push.h"
#include "include/http_transport.h"

static int push_branch(const char* url, const char* branch) {
    char local_ref[256];
    snprintf(local_ref, sizeof(local_ref), ".pit/refs/heads/%s", branch);

    char local_hash[41];
    if (read_hash_file(local_ref, local_hash) != 0) {
        fprintf(stderr, "error: no commits on '%s', nothing to push\n", branch);
        return 1;
    }

    // 1. upload all objects the remote doesn't have yet
    char dst_objects[512];
    snprintf(dst_objects, sizeof(dst_objects), "%s/.pit/objects/", url);
    char* rsync_args[] = {"rsync", "-a", "--ignore-existing",
                          ".pit/objects/", dst_objects, NULL};
    if (run_cmd(rsync_args) != 0) {
        fprintf(stderr, "error: failed to upload objects\n");
        return 1;
    }

    // 2. see where the remote branch currently is
    char remote_ref[512];
    snprintf(remote_ref, sizeof(remote_ref), "%s/.pit/refs/heads/%s", url, branch);

    char* scp_down[] = {"scp", "-q", remote_ref, ".pit/REMOTE_REF_TMP", NULL};
    if (run_cmd(scp_down) == 0) {
        char remote_hash[41];
        int ok = read_hash_file(".pit/REMOTE_REF_TMP", remote_hash);
        unlink(".pit/REMOTE_REF_TMP");

        if (ok == 0) {
            if (strcmp(remote_hash, local_hash) == 0) {
                printf("Everything up-to-date\n");
                return 0;
            }
            // remote must be an ancestor of us, otherwise it's not a fast-forward
            if (!object_exists(remote_hash) || !is_ancestor(remote_hash, local_hash)) {
                fprintf(stderr, "error: push rejected, remote has commits you don't have\n");
                fprintf(stderr, "hint: run 'pit pull' first\n");
                return 1;
            }
        }
    }
    // (if scp failed, the branch doesn't exist on the remote yet, so it's a new branch)

    // 3. move the remote branch pointer
    char* scp_up[] = {"scp", "-q", local_ref, remote_ref, NULL};
    if (run_cmd(scp_up) != 0) {
        fprintf(stderr, "error: failed to update remote ref\n");
        return 1;
    }

    printf("Pushed %s -> %s (%s)\n", branch, url, local_hash);
    return 0;
}

void pit_push(const char* remote_name) {
    char* url = remote_get_url(remote_name);
    if (!url) {
        fprintf(stderr, "error: remote '%s' not found. Use: pit remote add <name> <url>\n", remote_name);
        return;
    }
    char* branch = get_current_branch();
    if (strncmp(url, "http", 4) == 0)
        http_push(url, branch);
    else
        push_branch(url, branch);
    free(branch);
    free(url);
}