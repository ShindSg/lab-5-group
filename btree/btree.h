#pragma once

#include "../posting.h"


/*
 * Minimum degree of the B-tree.
 *
 * For BTREE_T = 3:
 * - each node can store at most 2 * T - 1 = 5 keys;
 * - each internal node can have at most 2 * T = 6 children;
 * - each non-root node must contain at least T - 1 = 2 keys.
 */
#define BTREE_T        3
#define BTREE_MAX_KEYS (2 * BTREE_T - 1)
#define BTREE_MAX_CH   (2 * BTREE_T)


/*
 * B-tree node.
 *
 * keys[i] and postings[i] form one logical pair:
 *
 *     keys[i] -> postings[i]
 *
 * where postings[i] is the posting list for keys[i].
 *
 * For an internal node with n keys, children are arranged as:
 *
 *     children[0], keys[0], children[1], keys[1], ...,
 *     keys[n - 1], children[n]
 *
 * Therefore, a node with n keys has n + 1 valid children
 * if it is not a leaf.
 */
typedef struct BTreeNode {
    char*             keys[BTREE_MAX_KEYS];     /* Sorted keys stored in this node. */
    Vector*           postings[BTREE_MAX_KEYS]; /* Posting lists corresponding to keys. */
    struct BTreeNode* children[BTREE_MAX_CH];   /* Child pointers for internal nodes. */
    int               n;                        /* Current number of keys in this node. */
    int               is_leaf;                  /* Non-zero if this node is a leaf. */
} BTreeNode;


/*
 * B-tree container.
 *
 * root points to the root node.
 * size stores the number of unique keys in the tree.
 */
typedef struct {
    BTreeNode* root;
    int        size;
} BTree;


/*
 * Creates an empty B-tree.
 *
 * Returns:
 *   Pointer to a newly allocated BTree on success;
 *   NULL on allocation failure.
 *
 * The returned tree must be released with freeBTree().
 */
BTree*  createBTree(void);


/*
 * Frees the entire B-tree.
 *
 * This function releases:
 * - all nodes;
 * - all keys;
 * - all posting lists;
 * - the BTree object itself.
 *
 * Passing NULL is allowed.
 */
void    freeBTree(BTree* tree);


/*
 * Inserts a key occurrence into the B-tree.
 *
 * If key does not exist yet:
 * - a new key is inserted;
 * - a new posting list is created;
 * - the given document entry is appended to that posting list;
 * - tree->size is increased by one.
 *
 * If key already exists:
 * - no new key is inserted;
 * - the document entry is appended to the existing posting list;
 * - tree->size is not changed.
 *
 * Invalid arguments are ignored.
 */
void    btreeInsert(BTree* tree, const char* key, int doc_id, const char* title);


/*
 * Searches for a key in the B-tree.
 *
 * Returns:
 *   Pointer to the posting list for the key if found;
 *   NULL if the key does not exist or arguments are invalid.
 *
 * The returned Vector is owned by the tree and must not be freed
 * by the caller.
 */
Vector* btreeSearch(const BTree* tree, const char* key);


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
 *
 * Invalid tree or visit arguments are ignored.
 */
void    btreeTraverse(
    const BTree* tree,
    void (*visit)(const char* key, Vector* postings, void* ctx),
    void* ctx
);
