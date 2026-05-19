// Полностью на ваше усмотрение (только переиспользуйте код из предыдущих лабораторных, если он вам подходит)
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include "rbtree.h"

// ======================================================

RBNode* createRBNode(RBColor color, const char* key, RBNode *parent, RBNode *left, RBNode *right, Vector *postings){
    /* создание нода */
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
    /* создание пустого дерева */
    RBTree *tree = (RBTree*)malloc(sizeof(RBTree)); // выделение памяти
    if(!tree) return NULL;

    tree->size = 0;
    tree->nil = createRBNode(RB_BLACK, NULL, NULL, NULL, NULL, NULL); // NIL-узел для дерева
    tree->nil->left = tree->nil;
    tree->nil->right = tree->nil;
    tree->nil->parent = tree->nil;

    tree->root = tree->nil;
    return tree;
}

void freeRBNode(RBNode *node, RBTree *tree){
    /* рекурсивное удаление заданного узла и всех от него исходящих */
    if(!node || node == tree->nil){
        return; // по достижении NIL-узла рекурсия останавливается
    }

    freeRBNode(node->left, tree); // рекурсия
    freeRBNode(node->right, tree);

    free(node->key); // освобождение текущего узла после выполнения рекурсии по нижестоящим
    vectorFree(node->postings);
    free(node);
}

void freeRBTree(RBTree* tree){
    /* удаление дерева */
    freeRBNode(tree->root, tree);
    freeRBNode(tree->nil, tree);
    free(tree);
    return;
}

// =======================================================

static void rbRotateLeft(RBTree *tree, RBNode *x) {
    /* левый поворот вокруг заданного узла */

    RBNode *y = x->right;

    x->right = y->left;
    if (y->left != tree->nil)
        y->left->parent = x;

    y->parent = x->parent;
    if (x->parent == tree->nil)
        tree->root = y;
    else if (x == x->parent->left)
        x->parent->left = y;
    else
        x->parent->right = y;

    y->left   = x;
    x->parent = y;
}

static void rbRotateRight(RBTree *tree, RBNode *y) {
    /* правый поворот вокруг заданного узла */

    RBNode *x = y->left;

    y->left = x->right;
    if (x->right != tree->nil)
        x->right->parent = y;

    x->parent = y->parent;
    if (y->parent == tree->nil)
        tree->root = x;
    else if (y == y->parent->right)
        y->parent->right = x;
    else
        y->parent->left = x;

    x->right  = y;
    y->parent = x;
}

static void rbFixup(RBTree *tree, RBNode *z) {
    /* балансировка дерева */

    while (z->parent->color == RB_RED) {
        if (z->parent == z->parent->parent->left) {
            RBNode *uncle = z->parent->parent->right;

            if (uncle->color == RB_RED) {
                z->parent->color         = RB_BLACK;
                uncle->color             = RB_BLACK;
                z->parent->parent->color = RB_RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->right) {
                    z = z->parent;
                    rbRotateLeft(tree, z);
                }
                z->parent->color         = RB_BLACK;
                z->parent->parent->color = RB_RED;
                rbRotateRight(tree, z->parent->parent);
            }
        } else {
            RBNode *uncle = z->parent->parent->left;

            if (uncle->color == RB_RED) {
                z->parent->color         = RB_BLACK;
                uncle->color             = RB_BLACK;
                z->parent->parent->color = RB_RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->left) {
                    z = z->parent;
                    rbRotateRight(tree, z);
                }
                z->parent->color         = RB_BLACK;
                z->parent->parent->color = RB_RED;
                rbRotateLeft(tree, z->parent->parent);
            }
        }
    }
    tree->root->color = RB_BLACK;
}

void rbInsert(RBTree *tree, const char *key, int doc_id, const char *title) {
    /* вставка узла */

    RBNode *parent  = tree->nil;
    RBNode *current = tree->root; // итеративный элемент

    while (current != tree->nil) { // цикл до достижения конца дерева
        int cmp = strcmp(key, current->key); // сравнение ключа с текущим
        if (cmp == 0) { // при равенстве добавление в постинглист узла
            PostingEntry entry;
            entry.doc_id = doc_id;
            strncpy(entry.title, title, MAX_TITLE_LEN - 1);
            entry.title[MAX_TITLE_LEN - 1] = '\0';
            appendVectorItem(current->postings, &entry);
            return;
        }
        parent  = current; // 
        current = (cmp < 0) ? current->left : current->right; // движение по дереву в соответствии с результатом сравнения
    }

    Vector *postings = createVector(sizeof(PostingEntry)); // создание постинглиста для нового узла
    if (!postings) return;

    PostingEntry entry;
    entry.doc_id = doc_id;
    strncpy(entry.title, title, MAX_TITLE_LEN - 1);
    entry.title[MAX_TITLE_LEN - 1] = '\0';
    appendVectorItem(postings, &entry);

    RBNode *z = createRBNode(RB_RED, key, parent, tree->nil, tree->nil, postings); // создание нового узла
    if (!z) { vectorFree(postings); return; }

    if (parent == tree->nil)
        tree->root = z;
    else if (strcmp(key, parent->key) < 0)
        parent->left  = z;
    else
        parent->right = z;

    tree->size++;

    rbFixup(tree, z); // Балансировка вокруг нового узла
}

Vector* rbSearch(const RBTree *tree, const char *key) {
    /* поиск по ключу */

    if (!tree || !key) return NULL;
 
    RBNode *current = tree->root; // итеративный элемент
 
    while (current != tree->nil) { // 
        int cmp = strcmp(key, current->key); // сравнение с искомым
        if (cmp == 0)
            return current->postings;
        current = (cmp < 0) ? current->left : current->right; // движение по дереву
    }
 
    return NULL;
}


// ==========================================================

void rbTraverse(
    const RBTree* tree,
    void (*visit)(const char* key, Vector* postings, void* ctx),
    void* ctx
)
{
    if (!tree || !visit) return;

    if (tree->root == tree->nil) return;

    RBNode** stack = malloc(sizeof(RBNode*) * (tree->size + 1));
    if (!stack) return;

    int top = -1;
    RBNode* current = tree->root;
    
    while (current != tree->nil || top >= 0) {
        while (current != tree->nil) {
            stack[++top] = current; // заполнение стека
            current = current->left; // движение влево
        }
        
        current = stack[top--];
        visit(current->key, current->postings, ctx); // применение функции
        current = current->right; // движение вправо
    }

    free(stack);
}


// ===========================================================

