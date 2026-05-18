#include "avl.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define _POSIX_C_SOURCE



// Helper utilities for AVL nodes

/*
 * Returns node height.
 *
 * NULL nodes are treated as height 0.
 */
static int node_height(const AVLNode* n) {
    return n ? n->height : 0;
}

/*
 * Returns maximum of two integer values.
 */
static int max2(int a, int b) {
    return a > b ? a : b;
}

/*
 * Computes AVL balance factor:
 *
 *     height(left) - height(right)
 *
 * Positive value means left-heavy subtree,
 * negative value means right-heavy subtree.
 */
static int balance_factor(const AVLNode* n) {
    return n ? node_height(n->left) - node_height(n->right) : 0;
}

/*
 * Recalculates node height after subtree modification.
 */
static void update_height(AVLNode* n) {
    n->height = 1 + max2(node_height(n->left), node_height(n->right));
}



// Tree creation / destruction

AVLTree* createAVLTree(void) {
    AVLTree* t = malloc(sizeof(AVLTree));
    if (!t) { perror("createAVLTree"); exit(EXIT_FAILURE); }

    /* New AVL tree starts empty */
    t->root = NULL;
    t->size = 0;

    return t;
}

/*
 * Recursively frees AVL subtree.
 *
 * Children are freed before the current node.
 */
static void free_node(AVLNode* n) {
    if (!n) return;

    /* Free children before freeing current node */
    free_node(n->left);
    free_node(n->right);

    /* Free key and posting list stored in current node */
    free(n->key);
    vectorFree(n->postings);

    free(n);
}

void freeAVLTree(AVLTree* tree) {
    if (!tree) return;

    free_node(tree->root);
    free(tree);
}



// AVL rotations

/*
 * Performs right rotation.
 *
 * Used to restore AVL balance in:
 * - Left-Left case
 * - Left-Right case
 */
static AVLNode* rotate_right(AVLNode* y) {
    /* Right rotation */
    AVLNode* x  = y->left;
    AVLNode* T2 = x->right;

    x->right = y;
    y->left  = T2;

    /* Heights must be updated bottom-up */
    update_height(y);
    update_height(x);

    return x;
}

/*
 * Performs left rotation.
 *
 * Used to restore AVL balance in:
 * - Right-Right case
 * - Right-Left case
 */
static AVLNode* rotate_left(AVLNode* x) {
    /* Left rotation */
    AVLNode* y  = x->right;
    AVLNode* T2 = y->left;

    y->left  = x;
    x->right = T2;

    /* Heights must be updated bottom-up */
    update_height(x);
    update_height(y);

    return y;
}



// Rebalancing

/*
 * Restores AVL balance property after insertion.
 *
 * Depending on subtree configuration, performs one of
 * four AVL rotation cases:
 *
 * - Left-Left
 * - Left-Right
 * - Right-Right
 * - Right-Left
 */
static AVLNode* rebalance(AVLNode* n) {
    /* Update height before checking balance */
    update_height(n);

    int bf = balance_factor(n);

    // Left-Left
    if (bf > 1 && balance_factor(n->left) >= 0)
        return rotate_right(n);

    // Left-Right
    if (bf > 1 && balance_factor(n->left) < 0) {
        n->left = rotate_left(n->left);
        return rotate_right(n);
    }

    // Right-Right
    if (bf < -1 && balance_factor(n->right) <= 0)
        return rotate_left(n);

    // Right-Left
    if (bf < -1 && balance_factor(n->right) > 0) {
        n->right = rotate_right(n->right);
        return rotate_left(n);
    }

    /* Node is already balanced */
    return n;
}



// Insertion

/*
 * Creates new AVL node with:
 * - copied key;
 * - initialized posting list;
 * - first document entry.
 */
static AVLNode* new_node(const char* key, int doc_id, const char* title) {
    AVLNode* n = malloc(sizeof(AVLNode));
    if (!n) { perror("new_node"); exit(EXIT_FAILURE); }

    /* Store own copy of key */
    n->key = strdup(key);
    if (!n->key) { perror("new_node: strdup"); exit(EXIT_FAILURE); }

    /* New AVL node is initially a leaf */
    n->height   = 1;
    n->left     = NULL;
    n->right    = NULL;

    /* Create posting list and append first document entry */
    n->postings = createPostingList();

    appendPosting(n->postings, doc_id, title);
    return n;
}

/*
 * Recursive AVL insertion.
 *
 * Returns new subtree root after possible rebalancing.
 */
static AVLNode* insert_rec(AVLNode* n, const char* key,
                            int doc_id, const char* title,
                            int* inserted) {
    /* Empty position found: create new node */
    if (!n) {
        *inserted = 1;
        return new_node(key, doc_id, title);
    }

    int cmp = strcmp(key, n->key);

    /* Insert into left subtree */
    if (cmp < 0)
        n->left  = insert_rec(n->left,  key, doc_id, title, inserted);

    /* Insert into right subtree */
    else if (cmp > 0)
        n->right = insert_rec(n->right, key, doc_id, title, inserted);

    else {
        /*
         * Key already exists:
         * append posting without changing tree structure.
         * Rebalancing is not needed in this case.
         */
        appendPosting(n->postings, doc_id, title);
        return n;
    }

    /* Restore AVL property after insertion */
    return rebalance(n);
}

void avlInsert(AVLTree* tree, const char* key, int doc_id, const char* title) {
    int inserted = 0;

    tree->root = insert_rec(tree->root, key, doc_id, title, &inserted);

    /* size stores number of unique keys */
    if (inserted) tree->size++;
}



// Search

Vector* avlSearch(const AVLTree* tree, const char* key) {
    if (!tree) return NULL;

    const AVLNode* cur = tree->root;

    while (cur) {
        int cmp = strcmp(key, cur->key);

        /* Target key is smaller: go left */
        if      (cmp < 0) cur = cur->left;

        /* Target key is greater: go right */
        else if (cmp > 0) cur = cur->right;

        /* Exact key found */
        else              return cur->postings;
    }

    /* Key does not exist */
    return NULL;
}



// Traversal

/*
 * Recursive in-order traversal:
 *
 *     left subtree -> current node -> right subtree
 *
 * Produces sorted key order.
 */
static void traverse_rec(const AVLNode* n,
                          void (*visit)(const char* key, Vector* postings, void* ctx),
                          void* ctx) {
    if (!n) return;

    /* In-order traversal: left subtree -> current node -> right subtree */
    traverse_rec(n->left, visit, ctx);
    visit(n->key, n->postings, ctx);
    traverse_rec(n->right, visit, ctx);
}

void avlTraverse(const AVLTree* tree,
                 void (*visit)(const char* key, Vector* postings, void* ctx),
                 void* ctx) {
    if (!tree) return;

    traverse_rec(tree->root, visit, ctx);
}
