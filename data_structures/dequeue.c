#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "../include/dequeue.h"

/**
 * @brief Creates a new deque node with the given prefix.
 *
 * @param prefix  Directory prefix string
 * @return        Heap-allocated DListNode, or NULL on failure
 */
DListNode* deq_createNode(const char* prefix) {
    DListNode *newNode = (DListNode*) malloc(sizeof(DListNode));
    if (!newNode) return NULL;
    newNode->prefix = strdup(prefix);
    newNode->hash = NULL;
    newNode->next = NULL;
    newNode->prev = NULL;
    return newNode;
}

void deq_init(Deque *deque) {
    deque->_head = deque->_tail = NULL;
    deque->_size = 0;
}

bool deq_isEmpty(Deque *deque) {
    return (deque->_head == NULL && deque->_tail == NULL);
}

/**
 * @brief Pushes a prefix to the front of the deque.
 *
 * @param deque   Deque to push onto
 * @param prefix  Directory prefix string
 */
void deq_pushFront(Deque *deque, const char* prefix) {
    DListNode *newNode = deq_createNode(prefix);
    if (newNode) {
        deque->_size++;
        if (deq_isEmpty(deque)) {
            deque->_head = deque->_tail = newNode;
            return;
        }
        newNode->next = deque->_head;
        deque->_head->prev = newNode;
        deque->_head = newNode;
    }
}

/**
 * @brief Pushes a prefix to the back of the deque.
 *
 * @param deque   Deque to push onto
 * @param prefix  Directory prefix string
 */
void deq_pushBack(Deque *deque, const char* prefix) {
    DListNode *newNode = deq_createNode(prefix);
    if (newNode) {
        deque->_size++;
        if (deq_isEmpty(deque)) {
            deque->_head = deque->_tail = newNode;
            return;
        }
        deque->_tail->next = newNode;
        newNode->prev = deque->_tail;
        deque->_tail = newNode;
    }
}

/**
 * @brief Returns the prefix at the front of the deque.
 *
 * @param deque  Deque to peek at
 * @return       Prefix string, or NULL if empty
 */
char* deq_front(Deque *deque) {
    if (!deq_isEmpty(deque))
        return deque->_head->prefix;
    return NULL;
}

/**
 * @brief Returns the prefix at the back of the deque.
 *
 * @param deque  Deque to peek at
 * @return       Prefix string, or NULL if empty
 */
char* deq_back(Deque *deque) {
    if (!deq_isEmpty(deque))
        return deque->_tail->prefix;
    return NULL;
}

/**
 * @brief Returns the front node (to access both prefix and hash).
 *
 * @param deque  Deque to peek at
 * @return       Front DListNode, or NULL if empty
 */
DListNode* deq_frontNode(Deque *deque) {
    return deque->_head;
}

void deq_popFront(Deque *deque) {
    if (!deq_isEmpty(deque)) {
        DListNode *temp = deque->_head;
        if (deque->_head == deque->_tail) {
            deque->_head = deque->_tail = NULL;
        } else {
            deque->_head = deque->_head->next;
            deque->_head->prev = NULL;
        }
        free(temp->prefix);
        free(temp->hash);
        free(temp);
        deque->_size--;
    }
}

void deq_popBack(Deque *deque) {
    if (!deq_isEmpty(deque)) {
        DListNode *temp = deque->_tail;
        if (deque->_head == deque->_tail) {
            deque->_head = deque->_tail = NULL;
        } else {
            deque->_tail = deque->_tail->prev;
            deque->_tail->next = NULL;
        }
        free(temp->prefix);
        free(temp->hash);
        free(temp);
        deque->_size--;
    }
}

int deq_size(Deque *deque) {
    return deque->_size;
}

/**
 * @brief Returns the back node.
 *
 * @param deque  Deque to peek at
 * @return       Back DListNode, or NULL if empty
 */
DListNode* deq_backNode(Deque *deque) {
    return deque->_tail;
}

/**
 * @brief Sets the hash on a node.
 *
 * @param node  Node to update
 * @param hash  Hash string to set
 */
void deq_setHash(DListNode *node, const char* hash) {
    if (node->hash) free(node->hash);
    node->hash = strdup(hash);
}

/**
 * @brief Sets the prefix on a node.
 *
 * @param node    Node to update
 * @param prefix  Prefix string to set
 */
void deq_setPrefix(DListNode *node, const char* prefix) {
    if (node->prefix) free(node->prefix);
    node->prefix = strdup(prefix);
}

/**
 * @brief Gets the hash from a node.
 *
 * @param node  Node to read from
 * @return      Hash string, or NULL if not set
 */
char* deq_getHash(DListNode *node) {
    return node->hash;
}

/**
 * @brief Gets the prefix from a node.
 *
 * @param node  Node to read from
 * @return      Prefix string
 */
char* deq_getPrefix(DListNode *node) {
    return node->prefix;
}
