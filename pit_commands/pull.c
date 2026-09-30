#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "include/fetch.h"
#include "include/merge.h"
#include "include/branch.h"
#include "include/checkout.h"
#include "include/transport.h"
#include "include/pull.h"

void pit_pull(const char* remote_name) {
    pit_fetch(remote_name);

    char* branch = get_current_branch();
    char local_ref[256], remote_ref[512];
    snprintf(local_ref, sizeof(local_ref), ".pit/refs/heads/%s", branch);
    snprintf(remote_ref, sizeof(remote_ref), ".pit/refs/remotes/%s/%s", remote_name, branch);

    // first pull into an empty repo: create the branch from the fetched ref
    if (access(local_ref, F_OK) != 0) {
        char hash[41];
        if (read_hash_file(remote_ref, hash) != 0) {
            fprintf(stderr, "error: nothing fetched for '%s'\n", branch);
            free(branch);
            return;
        }
        FILE* f = fopen(local_ref, "w");
        fprintf(f, "%s\n", hash);
        fclose(f);
        pit_checkout(branch);
        free(branch);
        return;
    }

    char target[512];
    snprintf(target, sizeof(target), "%s/%s", remote_name, branch);
    pit_merge(target);
    free(branch);
}