#ifndef PACK_H
#define PACK_H

#define OBJ_COMMIT 1
#define OBJ_TREE 2
#define OBJ_BLOB 3
#define OBJ_TAG 4
#define OBJ_OFS_DELTA 6
#define OBJ_REF_DELTA 7

/**
 * @brief One object read out of or written into a packfile.
 */
typedef struct {
    int type;                   /**< OBJ_COMMIT, OBJ_TREE, OBJ_BLOB, ... */
    int size;                   /**< Uncompressed size in bytes */
    int offset;                 /**< Where this object starts in the pack */
    unsigned char* content;     /**< Uncompressed bytes — owned by this struct */
} PackObject;

const char* get_type_name(int type);
int get_type_number(const char* type_name);
PackObject* read_pack(const char* path, int* count);
void free_pack(PackObject* objects, int count);
int write_pack(const char* path, PackObject* objects, int count);

#endif