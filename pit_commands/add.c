/**
 * @file add.c
 * @brief Implements the pit add command — hashing files into blob
 *        objects and recording them in the index.
 *
 * The index is a plain-text file of "<mode> <hash> <path>" lines, one
 * per staged file. Because a file's entry must be *replaced* when its
 * content changes, updates rewrite the index wholesale rather than
 * appending — it is small enough that this is simpler and safer than
 * seeking within it.
 */

#define _DEFAULT_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "include/hash_object.h"
#include "include/entry.h"
#include <dirent.h>
#include <stdbool.h>

#define MAX_INDEX_LINE 512

/**
 * @brief Outcome of staging a single file.
 */
typedef enum {
    INDEX_ADDED,     /**< Path was not in the index; a new entry was appended. */
    INDEX_UNCHANGED, /**< Path was already staged with an identical hash. */
    INDEX_UPDATED    /**< Path was staged with a different hash; entry replaced. */
} IndexResult;

/**
 * @brief Adds or updates one entry in .pit/index.
 *
 * Reads the whole index into memory, looks for an exact filename match,
 * and either appends, leaves an identical entry alone, or replaces the
 * hash of a modified one. Matching is on the full parsed filename
 * field, not a substring search — "a.txt" must not match "data.txt".
 *
 * @param mode      File mode as text, currently always "100644".
 * @param hash      40-character hex SHA-1 of the file's blob.
 * @param filename  Path relative to the repository root.
 * @return Which of the three cases applied.
 */
IndexResult update_index(const char* mode, const char* hash,
                         const char* filename) {
    char** lines = NULL;
    int count = 0, cap = 0;
    IndexResult result = INDEX_ADDED;

    FILE* file = fopen(".pit/index", "r");
    if (file != NULL) {
        char line[MAX_INDEX_LINE];
        while (fgets(line, sizeof(line), file) != NULL) {
            line[strcspn(line, "\n")] = '\0';
            if (line[0] == '\0') continue;

            /* Parse a copy so the original line survives intact for
               the rewrite below. */
            char copy[MAX_INDEX_LINE];
            snprintf(copy, sizeof(copy), "%s", line);
            char* entry_mode = strtok(copy, " ");
            char* entry_hash = strtok(NULL, " ");
            char* entry_name = strtok(NULL, "");
            if (entry_mode == NULL || entry_hash == NULL
                || entry_name == NULL) {
                continue;
            }

            if (strcmp(entry_name, filename) == 0) {
                if (strcmp(entry_hash, hash) == 0) {
                    result = INDEX_UNCHANGED;
                } else {
                    result = INDEX_UPDATED;
                    snprintf(line, sizeof(line), "%s %s %s",
                             mode, hash, filename);
                }
            }

            if (count >= cap) {
                cap = cap ? cap * 2 : 16;
                lines = realloc(lines, sizeof(char*) * cap);
            }
            lines[count++] = strdup(line);
        }
        fclose(file);
    }

    /* Nothing changed — leave the file untouched. */
    if (result == INDEX_UNCHANGED) {
        for (int i = 0; i < count; i++) free(lines[i]);
        free(lines);
        return INDEX_UNCHANGED;
    }

    FILE* out = fopen(".pit/index", "w");
    if (out == NULL) {
        perror(".pit/index");
        for (int i = 0; i < count; i++) free(lines[i]);
        free(lines);
        return INDEX_UNCHANGED;
    }

    for (int i = 0; i < count; i++) {
        fprintf(out, "%s\n", lines[i]);
        free(lines[i]);
    }
    free(lines);

    if (result == INDEX_ADDED) {
        fprintf(out, "%s %s %s\n", mode, hash, filename);
    }
    fclose(out);
    return result;
}

/**
 * @brief Hashes a single file and stages it.
 *
 * Stores the file's content as a blob object, then records or updates
 * its index entry. A leading "./" is stripped so paths are stored in
 * the same normalised form regardless of how add was invoked.
 *
 * @param filename  Path to the file to stage.
 */
void handle_one_file(const char* filename) {
    if (strncmp(filename, "./", 2) == 0) filename += 2;

    char* hashed_file = hash_file(filename);
    if (hashed_file == NULL) return;
    const char* mode = "100644";

    switch (update_index(mode, hashed_file, filename)) {
        case INDEX_ADDED:
            add_entry(mode, hashed_file, filename);
            printf("added %s\n", filename);
            break;
        case INDEX_UPDATED:
            add_entry(mode, hashed_file, filename);
            printf("modified %s\n", filename);
            break;
        case INDEX_UNCHANGED:
            printf("unchanged %s\n", filename);
            break;
    }
}

/**
 * @brief Reports whether a directory entry should be traversed.
 *
 * Filters out the self and parent links along with the repository
 * metadata directories, which must never be staged.
 *
 * @param filename  Bare directory entry name.
 * @return True if it should be visited.
 */
bool is_valid_file(const char* filename) {
    if (!strcmp(filename, "."))    return false;
    if (!strcmp(filename, ".."))   return false;
    if (!strcmp(filename, ".pit")) return false;
    if (!strcmp(filename, ".git")) return false;
    return true;
}

/**
 * @brief Recursively stages every regular file beneath a directory.
 *
 * @param prefix  Path relative to the repository root; empty string
 *                for the root itself.
 */
void handle_multiple_file(const char* prefix) {
    struct dirent* ent;
    DIR* dir = opendir(strlen(prefix) == 0 ? "." : prefix);
    if (dir == NULL) {
        perror("opendir");
        return;
    }

    while ((ent = readdir(dir)) != NULL) {
        if (!is_valid_file(ent->d_name)) {
            continue;
        }
        char full_path[512];
        if (strlen(prefix) == 0) {
            snprintf(full_path, sizeof(full_path), "%s", ent->d_name);
        } else {
            snprintf(full_path, sizeof(full_path), "%s/%s",
                     prefix, ent->d_name);
        }
        if (ent->d_type == DT_DIR) {
            handle_multiple_file(full_path);
        } else if (ent->d_type == DT_REG) {
            handle_one_file(full_path);
        }
    }

    closedir(dir);
}

/**
 * @brief Entry point for the pit add command.
 *
 * @param filename  File to stage, or "." to stage everything.
 */
void pit_add(const char* filename) {
    if (!strcmp(filename, ".")) {
        handle_multiple_file("");
    } else {
        handle_one_file(filename);
    }
}