#include <stdio.h>
#include <string.h>
#include "rbtree.h"

#define MAX_REC_DEPTH 512

// RB node/tree creation and cleanup

RBNode* createRBNode(RBColor color, const char* key, RBNode *parent, RBNode *left, RBNode *right, Vector *postings){
    /* Create and initialize a single RB node */
    RBNode *node = malloc(sizeof(RBNode));
    if(!node){ return NULL; }

    node->color = color;
    node->key = key ? strdup(key) : NULL;
    node->parent = parent ? parent : NULL;
    node->left = left ? left : NULL;
    node->right = right ? right : NULL;
    node->postings = postings ? postings : NULL;

    return node;
}

RBTree* createRBTree(void){
    /* Create an empty RB tree */
    RBTree *tree = (RBTree*)malloc(sizeof(RBTree));
    if(!tree) return NULL;

    tree->size = 0;

    /*
     * Sentinel NIL node replaces NULL children.
     * It simplifies rotations, fixup and traversal logic.
     */
    tree->nil = createRBNode(RB_BLACK, NULL, NULL, NULL, NULL, NULL);
    tree->nil->left = tree->nil;
    tree->nil->right = tree->nil;
    tree->nil->parent = tree->nil;

    tree->root = tree->nil;
    return tree;
}

void freeRBNode(RBNode *node, RBTree *tree){
    /* Recursively free the given node and all its descendants */
    if(!node || node == tree->nil){
        return;
    }

    /* Free children before the current node */
    freeRBNode(node->left, tree);
    freeRBNode(node->right, tree);

    /* Free data owned by the current node */
    free(node->key);
    vectorFree(node->postings);
    free(node);
}

void freeRBTree(RBTree* tree){
    /* Free the entire RB tree */

    freeRBNode(tree->root, tree);
    freeRBNode(tree->nil, tree);
    free(tree);
    return;
}

// RB rotations and balancing

/*
 * Performs left rotation around node x.
 *
 * x goes down to the left, and its right child
 * becomes the new root of this local subtree.
 */
static void rbRotateLeft(RBTree *tree, RBNode *x) {
    /* Left rotation around the given node */

    RBNode *y = x->right;

    /* Move y's left subtree to x's right subtree */
    x->right = y->left;
    if (y->left != tree->nil)
        y->left->parent = x;

    /* Connect y with x's former parent */
    y->parent = x->parent;
    if (x->parent == tree->nil)
        tree->root = y;
    else if (x == x->parent->left)
        x->parent->left = y;
    else
        x->parent->right = y;

    /* Put x below y */
    y->left   = x;
    x->parent = y;
}

/*
 * Performs right rotation around node y.
 *
 * y goes down to the right, and its left child
 * becomes the new root of this local subtree.
 */
static void rbRotateRight(RBTree *tree, RBNode *y) {
    /* Right rotation around the given node */

    RBNode *x = y->left;

    /* Move x's right subtree to y's left subtree */
    y->left = x->right;
    if (x->right != tree->nil)
        x->right->parent = y;

    /* Connect x with y's former parent */
    x->parent = y->parent;
    if (y->parent == tree->nil)
        tree->root = x;
    else if (y == y->parent->right)
        y->parent->right = x;
    else
        y->parent->left = x;

    /* Put y below x */
    x->right  = y;
    y->parent = x;
}

/*
 * Restores Red-Black tree properties after insertion.
 *
 * A newly inserted node is red, so the main possible violation
 * is a red node with a red parent.
 */
static void rbFixup(RBTree *tree, RBNode *z) {
    /* Balance the tree after insertion */

    while (z->parent->color == RB_RED) {
        if (z->parent == z->parent->parent->left) {
            RBNode *uncle = z->parent->parent->right;

            /* Case 1: parent and uncle are red */
            if (uncle->color == RB_RED) {
                z->parent->color         = RB_BLACK;
                uncle->color             = RB_BLACK;
                z->parent->parent->color = RB_RED;
                z = z->parent->parent;
            } else {
                /* Case 2: triangle shape, rotate parent first */
                if (z == z->parent->right) {
                    z = z->parent;
                    rbRotateLeft(tree, z);
                }

                /* Case 3: line shape, recolor and rotate grandparent */
                z->parent->color         = RB_BLACK;
                z->parent->parent->color = RB_RED;
                rbRotateRight(tree, z->parent->parent);
            }
        } else {
            RBNode *uncle = z->parent->parent->left;

            /* Mirror case 1: parent and uncle are red */
            if (uncle->color == RB_RED) {
                z->parent->color         = RB_BLACK;
                uncle->color             = RB_BLACK;
                z->parent->parent->color = RB_RED;
                z = z->parent->parent;
            } else {
                /* Mirror case 2: triangle shape */
                if (z == z->parent->left) {
                    z = z->parent;
                    rbRotateRight(tree, z);
                }

                /* Mirror case 3: line shape */
                z->parent->color         = RB_BLACK;
                z->parent->parent->color = RB_RED;
                rbRotateLeft(tree, z->parent->parent);
            }
        }
    }

    /* Root must always be black */
    tree->root->color = RB_BLACK;
}

// RB insertion

void rbInsert(RBTree *tree, const char *key, int doc_id, const char *title) {
    /* Insert key occurrence into the RB tree */

    RBNode *parent  = tree->nil;
    RBNode *current = tree->root;

    /*
     * Search for insertion position.
     * If key already exists, only append posting.
     */
    while (current != tree->nil) {
        int cmp = strcmp(key, current->key);

        if (cmp == 0) {
            PostingEntry entry;
            entry.doc_id = doc_id;
            strncpy(entry.title, title, MAX_TITLE_LEN - 1);
            entry.title[MAX_TITLE_LEN - 1] = '\0';
            appendVectorItem(current->postings, &entry);
            return;
        }

        parent  = current;
        current = (cmp < 0) ? current->left : current->right;
    }

    /* Create posting list for a new unique key */
    Vector *postings = createVector(sizeof(PostingEntry));
    if (!postings) return;

    PostingEntry entry;
    entry.doc_id = doc_id;
    strncpy(entry.title, title, MAX_TITLE_LEN - 1);
    entry.title[MAX_TITLE_LEN - 1] = '\0';
    appendVectorItem(postings, &entry);

    /* New RB node is inserted red */
    RBNode *z = createRBNode(RB_RED, key, parent, tree->nil, tree->nil, postings);
    if (!z) { vectorFree(postings); return; }

    /* Attach new node to its parent */
    if (parent == tree->nil)
        tree->root = z;
    else if (strcmp(key, parent->key) < 0)
        parent->left  = z;
    else
        parent->right = z;

    tree->size++;

    /* Restore Red-Black properties after insertion */
    rbFixup(tree, z);
}

Vector* rbSearch(const RBTree *tree, const char *key) {
    /* Search posting list by key */

    if (!tree || !key) return NULL;

    RBNode *current = tree->root;

    while (current != tree->nil) {
        int cmp = strcmp(key, current->key);

        if (cmp == 0)
            return current->postings;

        current = (cmp < 0) ? current->left : current->right;
    }

    return NULL;
}

// RB traversal

void rbTraverse(
    const RBTree* tree,
    void (*visit)(const char* key, Vector* postings, void* ctx),
    void* ctx
)
{
    /* Apply visit callback to all tree elements in sorted order */

    if (!tree || !visit) return;

    int top = -1; // iterative element
    RBNode* stack[tree->size]; // stack
    RBNode* current = tree->root; // iterative elemnt

    /*
     * Iterative in-order traversal:
     * left subtree -> node -> right subtree.
     */
    while (current != tree->nil || top >= 0) {
        while (current != tree->nil) {
            stack[++top] = current;
            current = current->left;
        }

        current = stack[top--];
        visit(current->key, current->postings, ctx);
        current = current->right;
    }
}
