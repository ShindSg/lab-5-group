#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdlib.h>

#include "btree.h"



/*
 * Creates empty B-tree node.
 * is_leaf:
 *   1 -> leaf node
 *   0 -> internal node
 */
static BTreeNode* createNode(int is_leaf)
{
    BTreeNode* node = malloc(sizeof(BTreeNode));
    if (!node) {
        return NULL;
    }

    node->is_leaf = is_leaf;
    node->n = 0;

    /* Initialize keys/postings with NULL */
    for (size_t i = 0; i < BTREE_MAX_KEYS; i++) {
        node->keys[i] = NULL;
        node->postings[i] = NULL;
    }

    /* Initialize children with NULL */
    for (size_t i = 0; i < BTREE_MAX_CH; i++) {
        node->children[i] = NULL;
    }

    return node;
}

BTree* createBTree(void)
{
    BTree* btree = malloc(sizeof(BTree));
    if (!btree) {
        return NULL;
    }

    BTreeNode* root = createNode(1);
    if (!root) {
        free(btree);
        return NULL;
    }

    btree->root = root;
    btree->size = 0;

    return btree;
}


/*
 * Splits full child node:
 *
 * child:
 * [k0 k1 k2 k3 k4]
 *
 * becomes:
 *
 * left child:
 * [k0 k1]
 *
 * median:
 * k2 -> parent
 *
 * right child:
 * [k3 k4]
 */
static void splitChild(BTreeNode* parent, int child_idx)
{
    if (!parent || !parent->children[child_idx]) {
        return;
    }

    BTreeNode* child = parent->children[child_idx];

    /* Create new right node */
    BTreeNode* right_node = createNode(child->is_leaf);
    if (!right_node) {
        return;
    }

    /* Move right half keys/postings into new node */
    for (int i = 0; i < BTREE_T - 1; i++) {
        right_node->keys[i] = child->keys[BTREE_T + i];
        right_node->postings[i] = child->postings[BTREE_T + i];
    }

    right_node->n = BTREE_T - 1;

    /* Move right half children if node is internal */
    if (!child->is_leaf) {
        for (int i = 0; i < BTREE_T; i++) {
            right_node->children[i] = child->children[BTREE_T + i];
        }
    }

    /* Save median key/posting */
    char* median_key = child->keys[BTREE_T - 1];
    Vector* median_posting = child->postings[BTREE_T - 1];

    /* Left node keeps only left half */
    child->n = BTREE_T - 1;

    /* Shift parent children to the right */
    for (int i = parent->n; i >= child_idx + 1; i--) {
        parent->children[i + 1] = parent->children[i];
    }

    /* Attach right node */
    parent->children[child_idx + 1] = right_node;

    /* Shift parent keys/postings */
    for (int i = parent->n - 1; i >= child_idx; i--) {
        parent->keys[i + 1] = parent->keys[i];
        parent->postings[i + 1] = parent->postings[i];
    }

    /* Insert median into parent */
    parent->keys[child_idx] = median_key;
    parent->postings[child_idx] = median_posting;

    parent->n++;

    /* Remove moved pointers from left node */
    child->keys[BTREE_T - 1] = NULL;
    child->postings[BTREE_T - 1] = NULL;

    for (int i = BTREE_T; i < BTREE_MAX_KEYS; i++) {
        child->keys[i] = NULL;
        child->postings[i] = NULL;
    }

    if (!child->is_leaf) {
        for (int i = BTREE_T; i < BTREE_MAX_CH; i++) {
            child->children[i] = NULL;
        }
    }
}

/*
 * Inserts key into node that is guaranteed
 * to be NOT full.
 */
static void insertNonFull(
    BTreeNode* node,
    const char* key,
    int doc_id,
    const char* title)
{
    if (!node) {
        return;
    }

    int i = node->n - 1;

    /*
     * Leaf insertion:
     * shift keys to make room for new key.
     */
    if (node->is_leaf) {
        while (i >= 0 && strcmp(key, node->keys[i]) < 0) {
            node->keys[i + 1] = node->keys[i];
            node->postings[i + 1] = node->postings[i];
            i--;
        }

        int position = i + 1;

        node->keys[position] = strdup(key);
        if (!node->keys[position]) {
            return;
        }

        node->postings[position] = createPostingList();
        if (!node->postings[position]) {
            free(node->keys[position]);
            node->keys[position] = NULL;
            return;
        }

        appendPosting(node->postings[position], doc_id, title);

        node->n++;
    } else {
        /* Find child subtree for insertion */
        while (i >= 0 && strcmp(key, node->keys[i]) < 0) {
            i--;
        }

        int child_idx = i + 1;

        /*
         * If target child is full,
         * split it before descending.
         */
        if (node->children[child_idx]->n == BTREE_MAX_KEYS) {
            splitChild(node, child_idx);

            /*
             * After split median moved up.
             * Decide whether to go left or right.
             */
            if (strcmp(key, node->keys[child_idx]) > 0) {
                child_idx++;
            }
        }

        insertNonFull(node->children[child_idx], key, doc_id, title);
    }
}

void btreeInsert(BTree* tree, const char* key, int doc_id, const char* title)
{
    if (!tree || !tree->root || !key || !title) {
        return;
    }

    /*
     * Duplicate key:
     * append posting instead of creating new key.
     */
    Vector* existing = btreeSearch(tree, key);
    if (existing) {
        appendPosting(existing, doc_id, title);
        return;
    }

    /*
     * Root is not full:
     * regular insertion.
     */
    if (tree->root->n < BTREE_MAX_KEYS) {
        insertNonFull(tree->root, key, doc_id, title);
    } else {
        /*
         * Root is full:
         * create new root and split old root.
         */
        BTreeNode* new_root = createNode(0);
        if (!new_root) {
            return;
        }

        new_root->children[0] = tree->root;

        splitChild(new_root, 0);
        tree->root = new_root;

        insertNonFull(new_root, key, doc_id, title);
    }

    tree->size++;
}


/*
 * Recursive B-tree search.
 */
static Vector* searchNode(const BTreeNode* node, const char* key)
{
    if (!node) {
        return NULL;
    }

    int i = 0;

    /* Find first key >= target */
    while (i < node->n && strcmp(key, node->keys[i]) > 0) {
        i++;
    }

    /* Key found */
    if (i < node->n && strcmp(key, node->keys[i]) == 0) {
        return node->postings[i];
    }

    /* Leaf reached -> key does not exist */
    if (node->is_leaf) {
        return NULL;
    }

    /* Continue search in child subtree */
    return searchNode(node->children[i], key);
}

Vector* btreeSearch(const BTree* tree, const char* key)
{
    if (!tree || !tree->root || !key) {
        return NULL;
    }

    return searchNode(tree->root, key);
}


/*
 * In-order traversal of B-tree.
 */
static void traverseNode(
    const BTreeNode* node,
    void (*visit)(const char* key, Vector* postings, void* ctx),
    void* ctx)
{
    if (!node) {
        return;
    }

    for (int i = 0; i < node->n; i++) {
        /* Visit left subtree */
        if (!node->is_leaf) {
            traverseNode(node->children[i], visit, ctx);
        }

        /* Visit current key */
        visit(node->keys[i], node->postings[i], ctx);
    }

    /* Visit rightmost subtree */
    if (!node->is_leaf) {
        traverseNode(node->children[node->n], visit, ctx);
    }
}

void btreeTraverse(
    const BTree* tree,
    void (*visit)(const char* key, Vector* postings, void* ctx),
    void* ctx)
{
    if (!tree || !tree->root || !visit) {
        return;
    }

    traverseNode(tree->root, visit, ctx);
}


/*
 * Recursively frees subtree.
 */
static void freeNode(BTreeNode* node)
{
    if (!node) {
        return;
    }

    /* Free children first */
    if (!node->is_leaf) {
        for (int i = 0; i <= node->n; i++) {
            freeNode(node->children[i]);
        }
    }

    /* Free keys/postings */
    for (int i = 0; i < node->n; i++) {
        free(node->keys[i]);
        vectorFree(node->postings[i]);
    }

    free(node);
}

void freeBTree(BTree* tree)
{
    if (!tree) {
        return;
    }

    freeNode(tree->root);
    free(tree);
}
