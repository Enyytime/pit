#ifndef DEQUEUE_H
#define DEQUEUE_H

#include <stdbool.h>

typedef struct dnode_t {
    char* prefix;   /**< Directory prefix e.g. "src" or "src/more" */
    char* hash;     /**< Tree hash after processing this prefix, NULL until built */
    struct dnode_t *next;
    struct dnode_t *prev;
} DListNode;

typedef struct deque_t {
    DListNode *_head;
    DListNode *_tail;
    unsigned _size;
} Deque;

DListNode* deq_createNode(const char* prefix);
void deq_init(Deque *deque);
bool deq_isEmpty(Deque *deque);
void deq_pushFront(Deque *deque, const char* prefix);
void deq_pushBack(Deque *deque, const char* prefix);
char* deq_front(Deque *deque);
char* deq_back(Deque *deque);
DListNode* deq_frontNode(Deque *deque);
DListNode* deq_backNode(Deque *deque);
void deq_popFront(Deque *deque);
void deq_popBack(Deque *deque);
int deq_size(Deque *deque);

/* Setters */
void deq_setHash(DListNode *node, const char* hash);
void deq_setPrefix(DListNode *node, const char* prefix);

/* Getters */
char* deq_getHash(DListNode *node);
char* deq_getPrefix(DListNode *node);

#endif
