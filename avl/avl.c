#define _POSIX_C_SOURCE 200809L

#include "avl.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Вспомогательные утилиты для узлов

static int node_height(const AVLNode* n) {
    return n ? n->height : 0;
}

static int max2(int a, int b) {
    return a > b ? a : b;
}

static int balance_factor(const AVLNode* n) {
    return n ? node_height(n->left) - node_height(n->right) : 0;
}

static void update_height(AVLNode* n) {
    n->height = 1 + max2(node_height(n->left), node_height(n->right));
}

// Создание / освобождение дерева
AVLTree* createAVLTree(void) {
    AVLTree* t = malloc(sizeof(AVLTree));
    if (!t) { perror("createAVLTree"); exit(EXIT_FAILURE); }
    t->root = NULL;
    t->size = 0;
    return t;
}

static void free_node(AVLNode* n) {
    if (!n) return;
    free_node(n->left);
    free_node(n->right);
    free(n->key);
    vectorFree(n->postings);
    free(n);
}

void freeAVLTree(AVLTree* tree) {
    if (!tree) return;
    free_node(tree->root);
    free(tree);
}

// Ротации
static AVLNode* rotate_right(AVLNode* y) {
    AVLNode* x  = y->left;
    AVLNode* T2 = x->right;

    x->right = y;
    y->left  = T2;

    update_height(y);
    update_height(x);
    return x;
}

static AVLNode* rotate_left(AVLNode* x) {
    AVLNode* y  = x->right;
    AVLNode* T2 = y->left;

    y->left  = x;
    x->right = T2;

    update_height(x);
    update_height(y);
    return y;
}

// Балансировка узла после вставки
static AVLNode* rebalance(AVLNode* n) {
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

    return n;
}

// Вставка
static AVLNode* new_node(const char* key, int doc_id, const char* title) {
    AVLNode* n = malloc(sizeof(AVLNode));
    if (!n) { perror("new_node"); exit(EXIT_FAILURE); }

    n->key = strdup(key);
    if (!n->key) { perror("new_node: strdup"); exit(EXIT_FAILURE); }
    n->height   = 1;
    n->left     = NULL;
    n->right    = NULL;
    n->postings = createPostingList();

    appendPosting(n->postings, doc_id, title);
    return n;
}

static AVLNode* insert_rec(AVLNode* n, const char* key,
                            int doc_id, const char* title,
                            int* inserted) {
    if (!n) {
        *inserted = 1;
        return new_node(key, doc_id, title);
    }

    int cmp = strcmp(key, n->key);
    if (cmp < 0)
        n->left  = insert_rec(n->left,  key, doc_id, title, inserted);
    else if (cmp > 0)
        n->right = insert_rec(n->right, key, doc_id, title, inserted);
    else {
        /* Ключ уже существует — просто дополняем posting list,
           структура дерева не меняется, ребалансировка не нужна */
        appendPosting(n->postings, doc_id, title);
        return n;
    }

    return rebalance(n);
}

void avlInsert(AVLTree* tree, const char* key, int doc_id, const char* title) {
    int inserted = 0;
    tree->root = insert_rec(tree->root, key, doc_id, title, &inserted);
    if (inserted) tree->size++;
}

// Поиск
Vector* avlSearch(const AVLTree* tree, const char* key) {
    if (!tree) return NULL;
    const AVLNode* cur = tree->root;
    while (cur) {
        int cmp = strcmp(key, cur->key);
        if      (cmp < 0) cur = cur->left;
        else if (cmp > 0) cur = cur->right;
        else              return cur->postings;
    }
    return NULL;
}

// Обход (in-order)
static void traverse_rec(const AVLNode* n,
                          void (*visit)(const char* key, Vector* postings, void* ctx),
                          void* ctx) {
    if (!n) return;
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
