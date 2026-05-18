#pragma once

#include "../posting.h"

/*
 * AVL tree node.
 *
 * Each node stores one unique key and its posting list:
 *
 *     key -> postings
 *
 * The AVL invariant keeps the height difference between
 * left and right subtrees no greater than 1.
 */
typedef struct AVLNode {
    char*           key;      /* Key stored in this node. */
    int             height;   /* Height of this node. */
    Vector*         postings; /* Posting list corresponding to key. */
    struct AVLNode* left;     /* Left subtree with keys smaller than key. */
    struct AVLNode* right;    /* Right subtree with keys greater than key. */
} AVLNode;

/*
 * AVL tree container.
 *
 * root points to the root node.
 * size stores the number of unique keys in the tree.
 */
typedef struct {
    AVLNode* root;
    int      size;
} AVLTree;

/*
 * Creates an empty AVL tree.
 *
 * Returns:
 *   Pointer to a newly allocated AVLTree.
 *
 * The returned tree must be released with freeAVLTree().
 */
AVLTree* createAVLTree(void);

/*
 * Frees the entire AVL tree.
 *
 * This function releases:
 * - all nodes;
 * - all keys;
 * - all posting lists;
 * - the AVLTree object itself.
 *
 * Passing NULL is allowed.
 */
void freeAVLTree(AVLTree* tree);

/*
 * Inserts a key occurrence into the AVL tree.
 *
 * If key does not exist yet:
 * - a new node is created;
 * - a new posting list is created;
 * - the given document entry is appended to that posting list;
 * - tree->size is increased by one.
 *
 * If key already exists:
 * - no new node is created;
 * - the document entry is appended to the existing posting list;
 * - tree->size is not changed.
 */
void avlInsert(AVLTree* tree, const char* key, int doc_id, const char* title);

/*
 * Searches for a key in the AVL tree.
 *
 * Returns:
 *   Pointer to the posting list for the key if found;
 *   NULL if the key does not exist or tree is NULL.
 *
 * The returned Vector is owned by the tree and must not be freed
 * by the caller.
 */
Vector* avlSearch(const AVLTree* tree, const char* key);

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
void avlTraverse(
    const AVLTree* tree,
    void (*visit)(const char* key, Vector* postings, void* ctx),
    void* ctx
);
