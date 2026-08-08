/**
 * @file refs.c
 * @brief Atomic, crash-safe updates to branch references.
 *
 * A reference is a file under .pit/refs/heads/ holding a single commit
 * hash. Writing it in place is unsafe: fopen("w") truncates before
 * anything is written, so a crash mid-update leaves an empty or partial
 * ref and an unrecoverable repository. Every update here instead writes
 * a sibling .lock file and rename()s it into place, which POSIX
 * guarantees is atomic — a concurrent reader sees either the whole old
 * value or the whole new one.
 *
 * The lock file doubles as a mutual-exclusion token: it is created with
 * O_EXCL, so exactly one writer can hold a given ref at a time. That
 * exclusivity is what makes the compare-and-swap in update_ref()
 * sound — the value cannot change between the check and the rename.
 */

#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include "include/refs.h"
#include <stdbool.h>

/**
 * @brief Reads the commit hash a branch points at.
 *
 * @param branch  Branch name, without any path prefix.
 * @return Malloc'd 40-character hex hash, or NULL if the branch does
 *         not exist or is unreadable. Caller frees.
 */
char* read_ref(const char* branch) {
    char path[512];
    snprintf(path, sizeof(path), ".pit/refs/heads/%s", branch);

    // open .pit/refs/heads/(branch name)
    FILE* f = fopen(path, "r");

    if (f == NULL) {
        return NULL;
    }

    char buf[HASH_HEX_LEN + 2];
    if (fgets(buf, sizeof(buf), f) == NULL) {
        fclose(f);
        return NULL;
    }
    fclose(f);

    // copy the hex "d1ecbd9f2a3b..."
    buf[strcspn(buf, "\r\n")] = '\0';
    if (strlen(buf) != HASH_HEX_LEN) {
        return NULL;
    }

    return strdup(buf);
}

/**
 * @brief Atomically sets a branch to a new commit, if unchanged.
 *
 * Acquires an exclusive lock, verifies the current value matches
 * @p expected_old_hash, then swaps the new value in with rename().
 * The compare-and-swap is what makes concurrent updates safe: a second
 * writer that read the same starting point is rejected rather than
 * silently discarding the first writers commit.
 *
 * @param branch             Branch name, without any path prefix.
 * @param new_hash           40-character hex hash to store.
 * @param expected_old_hash  Hash the caller believes is current, or
 *                           NULL to require that the branch not exist.
 * @return REF_OK on success, or a RefResult describing the failure.
 */
RefResult update_ref(const char* branch, const char* new_hash,
                     const char* expected_old_hash) {
    char path[512], lock_path[520];
    snprintf(path, sizeof(path), ".pit/refs/heads/%s", branch);
    snprintf(lock_path, sizeof(lock_path), "%s.lock", path);

    /* O_EXCL makes this the mutual-exclusion primitive: if the lock
       already exists, another writer owns this ref right now. */
    int fd = open(lock_path, O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (fd < 0) {
        return (errno == EEXIST) ? REF_LOCKED : REF_IO_ERROR;
    }

    /* Safe to read only now — holding the lock means the value cannot
       change between this check and the rename below. */
    char* current = read_ref(branch);
    bool matches;
    if (expected_old_hash == NULL) {
        matches = (current == NULL);
    } else {
        matches = (current != NULL
                   && strcmp(current, expected_old_hash) == 0);
    }
    free(current);

    if (!matches) {
        close(fd);
        unlink(lock_path);
        return REF_STALE;
    }

    char line[HASH_HEX_LEN + 2];
    int len = snprintf(line, sizeof(line), "%s\n", new_hash);
    if (write(fd, line, len) != len) {
        close(fd);
        unlink(lock_path);
        return REF_IO_ERROR;
    }

    /* Force to disk before the rename, so a crash cannot leave the ref
       pointing at a file whose contents never landed. */
    if (fsync(fd) != 0) {
        close(fd);
        unlink(lock_path);
        return REF_IO_ERROR;
    }
    close(fd);

    if (rename(lock_path, path) != 0) {
        unlink(lock_path);
        return REF_IO_ERROR;
    }
    return REF_OK;
}


/**
 * @brief Reads the branch name HEAD currently points at.
 *
 * @return Malloc'd branch name, or NULL if HEAD is missing or detached.
 *         Caller frees.
 */

char* current_branch() {
    FILE* f = fopen(".pit/HEAD", "r");

    if (f == NULL) {
        return NULL;
    }

    char buf[512];
    if (fgets(buf, sizeof(buf), f) == NULL) {
        fclose(f);
        return NULL;
    }
    fclose(f);

    buf[strcspn(buf, "\r\n")] = '\0';
    
    const char* prefix = "ref: refs/heads/";
    if (strncmp(buf, prefix, strlen(prefix)) != 0) {
        return NULL;
    }

    int branch_name_start = strlen(prefix);
    return strdup(buf + branch_name_start);
}

