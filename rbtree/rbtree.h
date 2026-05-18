#pragma once

#include "../posting.h"

/*
 * Red-Black node color.
 */
typedef enum {
    RB_RED,
    RB_BLACK
} RBColor;

/*
 * Red-Black tree node.
 *
 * Each node stores one unique key and its posting list:
 *
 *     key -> postings
 *
 * left and right point to child nodes.
 * parent is used for rotations and insertion fixup.
 */
typedef struct RBNode {
    char*           key;      /* Key stored in this node. */
    RBColor         color;    /* Node color: red or black. */
    Vector*         postings; /* Posting list corresponding to key. */
    struct RBNode*  left;     /* Left subtree with smaller keys. */
    struct RBNode*  right;    /* Right subtree with greater keys. */
    struct RBNode*  parent;   /* Parent node. */
} RBNode;

/*
 * Red-Black tree container.
 *
 * root points to the root node.
 * nil is a shared black sentinel node used instead of NULL leaves.
 * size stores the number of unique keys in the tree.
 */
typedef struct {
    RBNode* root;
    RBNode* nil;   /* Black sentinel leaf node. */
    int     size;
} RBTree;

/*
 * Creates an empty Red-Black tree.
 *
 * Returns:
 *   Pointer to a newly allocated RBTree.
 *
 * The returned tree must be released with freeRBTree().
 */
RBTree* createRBTree(void);

/*
 * Frees the entire Red-Black tree.
 *
 * This function releases:
 * - all regular nodes;
 * - all keys;
 * - all posting lists;
 * - sentinel NIL node;
 * - the RBTree object itself.
 */
void freeRBTree(RBTree* tree);

/*
 * Inserts a key occurrence into the Red-Black tree.
 *
 * If key does not exist yet:
 * - a new red node is inserted;
 * - a new posting list is created;
 * - the given document entry is appended;
 * - tree->size is increased by one;
 * - Red-Black properties are restored.
 *
 * If key already exists:
 * - no new node is inserted;
 * - the document entry is appended to the existing posting list;
 * - tree->size is not changed.
 */
void rbInsert(RBTree* tree, const char* key, int doc_id, const char* title);

/*
 * Searches for a key in the Red-Black tree.
 *
 * Returns:
 *   Pointer to the posting list for the key if found;
 *   NULL if the key does not exist or arguments are invalid.
 *
 * The returned Vector is owned by the tree and must not be freed
 * by the caller.
 */
Vector* rbSearch(const RBTree* tree, const char* key);

/*
 * Traverses all keys in sorted order.
 *
 * For every key, calls:
 *
 *     visit(key, postings, ctx)
 *
 * where:
 * - key is the current key;
 * - postings is the posting list for that key;
 * - ctx is user-provided context passed through unchanged.
 */
void rbTraverse(
    const RBTree* tree,
    void (*visit)(const char* key, Vector* postings, void* ctx),
    void* ctx
);
