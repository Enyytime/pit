#ifndef PACK_H
#define PACK_H

#define OBJ_COMMIT 1
#define OBJ_TREE 2
#define OBJ_BLOB 3
#define OBJ_TAG 4
#define OBJ_OFS_DELTA 6
#define OBJ_REF_DELTA 7

typedef struct {
    int type;
    int size;
    int offset;
    unsigned char* content;
} PackObject;

const char* get_type_name(int type);
PackObject* read_pack(const char* path, int* count);
void free_pack(PackObject* objects, int count);

#endif