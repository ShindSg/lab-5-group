#pragma once

#include "../posting.h"

/*
 * Supported tree implementations
 * used as index backend.
 */
typedef enum {
    TREE_AVL,    /* AVL tree implementation */
    TREE_RB,     /* Red-Black tree implementation */
    TREE_BTREE   /* B-tree implementation */
} TreeType;

/*
 * Generic index abstraction.
 *
 * tree points to concrete tree implementation:
 * - AVLTree
 * - RBTree
 * - BTree
 *
 * type stores which implementation is currently used.
 */
typedef struct {
    void*    tree;
    TreeType type;
} Index;

/*
 * Creates new index with selected tree backend.
 *
 * Depending on type, internally creates:
 * - AVL tree;
 * - Red-Black tree;
 * - B-tree.
 *
 * Returns:
 *   Pointer to newly allocated Index structure.
 */
Index* createIndex(TreeType type);

/*
 * Inserts term occurrence into index.
 *
 * If term already exists:
 * - posting is appended to existing posting list.
 *
 * Otherwise:
 * - new tree node is created;
 * - new posting list is initialized.
 */
void insertTerm(Index* idx, const char* term, int doc_id, const char* title);

/*
 * Searches for term inside index.
 *
 * Returns:
 *   Pointer to posting list if term exists;
 *   NULL otherwise.
 *
 * Returned Vector is owned by the index
 * and must not be freed by caller.
 */
Vector* lookupTerm(const Index* idx, const char* term);

/*
 * Indexes all tokens belonging to one document.
 *
 * Each token is inserted independently
 * into the underlying tree structure.
 */
void indexDocument(Index* idx, int doc_id, const char* title,
                   const char** tokens, int n_tokens);

/*
 * Traverses index in sorted key order.
 *
 * For every term calls:
 *
 *     visit(key, postings, ctx)
 *
 * where:
 * - key is current indexed term;
 * - postings is corresponding posting list;
 * - ctx is user-provided context pointer.
 */
void traverseIndex(
    const Index* idx,
    void (*visit)(const char* key, Vector* postings, void* ctx),
    void* ctx
);

/*
 * Saves index contents into text file.
 *
 * Serialized format:
 *
 *     term posting_count
 *     doc_id title
 *     doc_id title
 *     ...
 */
void saveIndex(const Index* idx, const char* path);

/*
 * Loads index from serialized text file.
 *
 * Tree implementation is selected
 * using provided TreeType.
 *
 * Returns:
 *   Newly allocated Index on success;
 *   NULL on failure.
 */
Index* loadIndex(const char* path, TreeType type);

/*
 * Frees entire index structure,
 * including underlying tree backend.
 *
 * Passing NULL is allowed.
 */
void freeIndex(Index* idx);
