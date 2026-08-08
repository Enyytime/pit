/**
 * @file write_tree.c
 * @brief Builds Git-compatible tree objects from the pit index.
 *
 * The index is a flat list of "<mode> <hash> <path>" lines, but tree
 * objects are hierarchical: one object per directory, each referencing
 * its children by hash. This file bridges the two by discovering every
 * directory mentioned in the index, processing them deepest-first so a
 * parent always has its children's hashes available, and finally
 * assembling the root tree.
 */

#define _DEFAULT_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "include/hash_object.h"
#include "include/entry.h"
#include "include/dequeue.h"
#include <dirent.h>
#include <stdbool.h>

/**
 * @brief One entry within a single tree object.
 *
 * Collected unsorted, then ordered by tree_entry_cmp() before
 * serialisation. @c is_dir is kept separately from @c mode because
 * Git's sort order depends on it.
 */
typedef struct {
    const char* mode;   /**< "100644" for files, "40000" for directories. */
    char* name;         /**< Entry name only, no path. Owned. */
    char* hash;         /**< 40-char hex SHA-1. Owned. */
    bool is_dir;        /**< Affects sort order — see tree_entry_cmp(). */
} TreeEntry;

/**
 * @brief A directory discovered while scanning the index.
 */
typedef struct directory_depth {
    int depth;          /**< 1 for a top-level directory, 2 for its child, etc. */
    char* parent;       /**< Parent's name, or NULL at depth 1. Owned. */
    char* prefix;       /**< This directory's own name. Owned. */
    char* full_path;    /**< Path from the repo root. Owned. */
    char* hash;         /**< Tree hash; NULL until this directory is processed. */
} Dir_depth;

/**
 * @brief qsort comparator ordering directories deepest-first.
 *
 * Guarantees a directory's children are hashed before it is, so their
 * hashes are available when its own tree object is built.
 *
 * @param a  Pointer to a Dir_depth.
 * @param b  Pointer to a Dir_depth.
 * @return Negative, zero or positive for descending depth order.
 */
int compare_depth(const void* a, const void* b) {
    Dir_depth* da = (Dir_depth*)a;
    Dir_depth* db = (Dir_depth*)b;
    return db->depth - da->depth;
}

/**
 * @brief Serialises one tree entry into Git's binary format.
 *
 * Produces "<mode> <name>\0<20-byte binary SHA-1>" — note the hash is
 * raw bytes, not hex, and the record has no trailing separator.
 *
 * @param mode         File mode as text ("100644" or "40000").
 * @param hash         40-character hex SHA-1.
 * @param filename     Entry name, without any path component.
 * @param line_length  Out: byte length of the returned buffer.
 * @return Malloc'd buffer, not NUL-terminated. Caller frees.
 */
char* build_tree_entry(const char* mode, const char* hash,
                       const char* filename, int* line_length) {
    unsigned char bin[20];
    for (int i = 0; i < 20; i++) {
        sscanf(hash + (i * 2), "%2hhx", &bin[i]);
    }

    int entry_len = strlen(mode) + 1 + strlen(filename) + 1 + 20;
    char* return_string = (char*)malloc(sizeof(char) * entry_len);
    int offset = 0;
    memcpy(return_string + offset, mode, strlen(mode));
    offset += strlen(mode);
    return_string[offset++] = ' ';
    memcpy(return_string + offset, filename, strlen(filename));
    offset += strlen(filename);
    return_string[offset++] = '\0';
    memcpy(return_string + offset, bin, 20);

    *line_length = entry_len;
    return return_string;
}

/**
 * @brief qsort comparator implementing Git's tree entry ordering.
 *
 * Git sorts by name, but compares directories as though they ended in
 * '/'. Since '.' (0x2E) is below '/' (0x2F) which is below every
 * alphanumeric, "src.c" sorts before "src/" while "srca" sorts after.
 * A plain strcmp on the bare name gets this wrong, and git fsck
 * reports the result as "treeNotSorted".
 *
 * @param a  Pointer to a TreeEntry.
 * @param b  Pointer to a TreeEntry.
 * @return Negative, zero or positive per Git's ordering.
 */
int tree_entry_cmp(const void* a, const void* b) {
    const TreeEntry* ea = (const TreeEntry*)a;
    const TreeEntry* eb = (const TreeEntry*)b;

    size_t la = strlen(ea->name);
    size_t lb = strlen(eb->name);
    size_t min = la < lb ? la : lb;

    int c = memcmp(ea->name, eb->name, min);
    if (c != 0) return c;

    /* One name is a prefix of the other: compare the byte that would
       follow, treating a directory's as '/' and a file's as '\0'. */
    unsigned char na = (la > min) ? (unsigned char)ea->name[min]
                                  : (ea->is_dir ? '/' : '\0');
    unsigned char nb = (lb > min) ? (unsigned char)eb->name[min]
                                  : (eb->is_dir ? '/' : '\0');
    return (int)na - (int)nb;
}

/**
 * @brief Appends an entry to a growable TreeEntry array.
 *
 * Copies @p name and @p hash; the caller's buffers may be reused or
 * freed afterwards. @p mode is stored by pointer and must outlive the
 * array (string literals only).
 *
 * @param entries  In/out: array pointer, reallocated as needed.
 * @param count    In/out: number of entries currently stored.
 * @param cap      In/out: allocated capacity.
 * @param mode     File mode literal.
 * @param hash     40-character hex SHA-1.
 * @param name     Entry name, without any path component.
 * @param is_dir   True if this entry is a subdirectory.
 */
void add_tree_entry(TreeEntry** entries, int* count, int* cap,
                    const char* mode, const char* hash,
                    const char* name, bool is_dir) {
    if (*count >= *cap) {
        *cap = *cap ? *cap * 2 : 16;
        *entries = realloc(*entries, sizeof(TreeEntry) * (*cap));
    }
    (*entries)[*count].mode   = mode;
    (*entries)[*count].name   = strdup(name);
    (*entries)[*count].hash   = strdup(hash);
    (*entries)[*count].is_dir = is_dir;
    (*count)++;
}

/**
 * @brief Sorts entries and serialises them into a tree object body.
 *
 * Sorting happens here rather than at insertion so callers can add
 * files and subdirectories in whatever order is convenient.
 *
 * @param entries  Entry array; reordered in place.
 * @param count    Number of entries.
 * @param out_len  Out: byte length of the returned buffer.
 * @return Malloc'd tree body, not NUL-terminated. Caller frees.
 */
char* build_tree_object(TreeEntry* entries, int count, int* out_len) {
    qsort(entries, count, sizeof(TreeEntry), tree_entry_cmp);

    int total = 0;
    for (int i = 0; i < count; i++) {
        total += strlen(entries[i].mode) + 1
               + strlen(entries[i].name) + 1 + 20;
    }

    char* buf = malloc(total > 0 ? total : 1);
    int offset = 0;
    for (int i = 0; i < count; i++) {
        int line_length;
        char* entry = build_tree_entry(entries[i].mode, entries[i].hash,
                                       entries[i].name, &line_length);
        memcpy(buf + offset, entry, line_length);
        offset += line_length;
        free(entry);
    }
    *out_len = offset;
    return buf;
}

/**
 * @brief Frees a TreeEntry array and the strings it owns.
 *
 * @param entries  Array to free. May be NULL if @p count is 0.
 * @param count    Number of entries.
 */
void free_tree_entries(TreeEntry* entries, int count) {
    for (int i = 0; i < count; i++) {
        free(entries[i].name);
        free(entries[i].hash);
    }
    free(entries);
}

/**
 * @brief Splits a path into components, dropping the final filename.
 *
 * Destructive: overwrites each '/' in @p str with '\0', so the returned
 * pointers alias into the caller's buffer and must not outlive it.
 * "src/deep/note.txt" yields {"src", "deep"}.
 *
 * @param str        Path to split, modified in place.
 * @param numFields  Out: number of directory components returned.
 * @return Malloc'd NULL-terminated array of pointers into @p str.
 *         Free the array itself, never its elements.
 */
char** split_string_on_slash(char* str, size_t* numFields) {
    *numFields = 0;
    int capacity = 10;
    char** fields = malloc(sizeof(char*) * (capacity + 1));

    fields[(*numFields)++] = str;

    for (char* p = str; *p != '\0'; p++) {
        if (*p == '/') {
            *p = '\0';
            if ((int)(*numFields) >= capacity) {
                capacity *= 2;
                fields = realloc(fields, sizeof(char*) * (capacity + 1));
            }
            fields[(*numFields)++] = p + 1;
        }
    }

    /* Drop the trailing filename — only directories are wanted. */
    (*numFields)--;
    fields[*numFields] = NULL;
    return fields;
}

/**
 * @brief Counts path separators to determine nesting depth.
 *
 * @param filename  Path relative to the repository root.
 * @return 0 for a root-level file, 1 for a file one directory down, etc.
 */
int get_depth(const char* filename) {
    int count = 0;
    int len = strlen(filename);
    for (int i = 0; i < len; i++) {
        char c = filename[i];
        if (c == '/') {
            count++;
        }
    }
    return count;
}

/**
 * @brief Scans the index and collects every directory it mentions.
 *
 * Each path contributes an entry per component, deduplicated by full
 * path. Hashes are left NULL and filled in later as each directory's
 * tree object is written.
 *
 * @param file            Open index handle; read from the current offset.
 * @param dir_length_out  Out: number of directories found.
 * @return Malloc'd array of Dir_depth. Caller frees the array and each
 *         entry's owned strings.
 */
Dir_depth* put_index_to_dirs(FILE* file, int* dir_length_out) {
    int dir_capacity = 10;
    int dir_length = 0;

    Dir_depth* dirs = (Dir_depth*)malloc(sizeof(Dir_depth) * dir_capacity);

    char line[256];
    while (fgets(line, sizeof(line), file) != NULL) {
        strtok(line, " ");
        strtok(NULL, " ");
        char* filename = strtok(NULL, "\n");
        if (filename == NULL) continue;
        if (strncmp(filename, "./", 2) == 0) filename += 2;

        int depth = get_depth(filename);

        if (depth == 0) {
            continue;
        }

        size_t numFields;
        char** parts = split_string_on_slash(filename, &numFields);

        for (int i = 0; i < (int)numFields; i++) {
            char full_path[256] = "";

            for (int j = 0; j <= i; j++) {

                if (j > 0) {
                    strcat(full_path, "/");
                }

                strcat(full_path, parts[j]);
            }

            bool found = false;
            for (int k = 0; k < dir_length; k++) {

                if (strcmp(dirs[k].full_path, full_path) == 0) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                if (dir_length >= dir_capacity) {
                    dir_capacity *= 2;
                    dirs = realloc(dirs, sizeof(Dir_depth) * dir_capacity);
                }
                dirs[dir_length].full_path = strdup(full_path);
                dirs[dir_length].hash = NULL;
                dirs[dir_length].depth = i + 1;
                dirs[dir_length].parent = (i == 0) ? NULL : strdup(parts[i - 1]);
                dirs[dir_length].prefix = strdup(parts[i]);
                dir_length++;
            }
        }
        free(parts);
    }
    *dir_length_out = dir_length;
    return dirs;
}

/**
 * @brief Builds the full tree hierarchy from the index and stores it.
 *
 * Subdirectories are written deepest-first so every parent can
 * reference its children by hash, then the root tree is assembled from
 * root-level files plus the depth-1 directories.
 *
 * @return Malloc'd 40-character hex hash of the root tree, or NULL if
 *         no index exists. Caller frees.
 */
char* read_index(void) {
    FILE* file = fopen(".pit/index", "r");
    if (file == NULL) return NULL;

    char line[256];
    int dir_length;
    Dir_depth* dirs = put_index_to_dirs(file, &dir_length);
    qsort(dirs, dir_length, sizeof(Dir_depth), compare_depth);

    for (int i = 0; i < dir_length; i++) {
        TreeEntry* entries = NULL;
        int count = 0, cap = 0;

        /* Files sitting directly in this directory. */
        fseek(file, 0, SEEK_SET);
        while (fgets(line, sizeof(line), file) != NULL) {
            char* mode     = strtok(line, " ");
            char* hash     = strtok(NULL, " ");
            char* filename = strtok(NULL, "\n");
            if (mode == NULL || hash == NULL || filename == NULL) continue;
            if (strncmp(filename, "./", 2) == 0) filename += 2;

            size_t full_path_len = strlen(dirs[i].full_path);
            if (strncmp(filename, dirs[i].full_path, full_path_len)) continue;
            if (filename[full_path_len] != '/') continue;

            char* direct_child = filename + full_path_len + 1;
            if (strchr(direct_child, '/') != NULL) continue;

            add_tree_entry(&entries, &count, &cap,
                           "100644", hash, direct_child, false);
        }

        /* Immediate subdirectories, already hashed by the depth ordering. */
        for (int j = 0; j < dir_length; j++) {
            if (dirs[j].hash == NULL)   continue;
            if (dirs[j].parent == NULL) continue;
            if (strcmp(dirs[j].parent, dirs[i].prefix) != 0) continue;
            if (dirs[j].depth != dirs[i].depth + 1) continue;

            add_tree_entry(&entries, &count, &cap,
                           "40000", dirs[j].hash, dirs[j].prefix, true);
        }

        int subtree_len;
        char* subtree = build_tree_object(entries, count, &subtree_len);
        dirs[i].hash = store_object("tree", (unsigned char*)subtree,
                                    subtree_len);
        free(subtree);
        free_tree_entries(entries, count);
    }

    /* Root tree: root-level files plus every depth-1 directory. */
    TreeEntry* entries = NULL;
    int count = 0, cap = 0;

    fseek(file, 0, SEEK_SET);
    while (fgets(line, sizeof(line), file) != NULL) {
        char* mode     = strtok(line, " ");
        char* hash     = strtok(NULL, " ");
        char* filename = strtok(NULL, "\n");
        if (mode == NULL || hash == NULL || filename == NULL) continue;
        if (strncmp(filename, "./", 2) == 0) filename += 2;
        if (strchr(filename, '/') != NULL) continue;

        add_tree_entry(&entries, &count, &cap,
                       "100644", hash, filename, false);
    }

    for (int i = 0; i < dir_length; i++) {
        if (dirs[i].depth != 1) continue;
        add_tree_entry(&entries, &count, &cap,
                       "40000", dirs[i].hash, dirs[i].prefix, true);
    }

    int tree_len;
    char* tree = build_tree_object(entries, count, &tree_len);
    char* hex = store_object("tree", (unsigned char*)tree, tree_len);

    free(tree);
    free_tree_entries(entries, count);

    for (int i = 0; i < dir_length; i++) {
        free(dirs[i].full_path);
        free(dirs[i].prefix);
        free(dirs[i].parent);
        free(dirs[i].hash);
    }
    free(dirs);

    fclose(file);
    return hex;
}

/**
 * @brief Entry point for the pit write-tree command.
 *
 * Writes the tree hierarchy from the index and prints the root hash.
 */
void pit_write_tree(void) {
    char* tree = read_index();
    if (tree == NULL) {
        fprintf(stderr, "no index to write\n");
        return;
    }
    printf("%s\n", tree);
    free(tree);
}