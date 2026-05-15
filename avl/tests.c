/*
 * tests.c — unit-тесты для AVL-дерева
 *
 * Сборка:
 *   gcc -Wall -Wextra -o test_avl tests.c avl.c ../posting.c -I..
 *   ./test_avl
 *
 * Вывод:
 *   [PASS] / [FAIL] для каждого теста
 *   Итоговый счёт: N/M passed
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "avl.h"

/* ──────────────────────────────────────────────
   Мини-фреймворк
   ────────────────────────────────────────────── */

static int g_passed = 0;
static int g_total  = 0;

#define CHECK(cond) do {                                           \
    g_total++;                                                     \
    if (cond) { g_passed++; printf("[PASS] %s\n", #cond); }       \
    else       printf("[FAIL] %s  (line %d)\n", #cond, __LINE__); \
} while(0)

/* ──────────────────────────────────────────────
   Вспомогательные функции
   ────────────────────────────────────────────── */

/* Проверяет AVL-инвариант рекурсивно; возвращает высоту или -1 при ошибке */
static int check_avl_invariant(const AVLNode* n) {
    if (!n) return 0;

    int lh = check_avl_invariant(n->left);
    int rh = check_avl_invariant(n->right);
    if (lh < 0 || rh < 0) return -1;

    int diff = lh - rh;
    if (diff < -1 || diff > 1) return -1;   /* нарушение AVL */

    int expected_height = 1 + (lh > rh ? lh : rh);
    if (n->height != expected_height) return -1;  /* высота не согласована */

    return expected_height;
}

/* Проверяет BST-порядок рекурсивно */
static int check_bst_order(const AVLNode* n,
                            const char* lo, const char* hi) {
    if (!n) return 1;
    if (lo && strcmp(n->key, lo) <= 0) return 0;
    if (hi && strcmp(n->key, hi) >= 0) return 0;
    return check_bst_order(n->left,  lo, n->key)
        && check_bst_order(n->right, n->key, hi);
}

/* Контекст для подсчёта узлов при обходе */
typedef struct { int count; } TraverseCtx;

static void count_visitor(const char* key, Vector* postings, void* ctx) {
    (void)key; (void)postings;
    ((TraverseCtx*)ctx)->count++;
}

/* ──────────────────────────────────────────────
   ТЕСТ 1: Создание и освобождение пустого дерева
   ────────────────────────────────────────────── */
static void test_create_free(void) {
    printf("\n=== test_create_free ===\n");
    AVLTree* t = createAVLTree();
    CHECK(t != NULL);
    CHECK(t->root == NULL);
    CHECK(t->size == 0);
    freeAVLTree(t);
    CHECK(1); /* нет краша — дерево освободилось корректно */
}

/* ──────────────────────────────────────────────
   ТЕСТ 2: Вставка одного ключа
   ────────────────────────────────────────────── */
static void test_single_insert(void) {
    printf("\n=== test_single_insert ===\n");
    AVLTree* t = createAVLTree();

    avlInsert(t, "python", 1, "How to use Python?");

    CHECK(t->size == 1);
    CHECK(t->root != NULL);
    CHECK(strcmp(t->root->key, "python") == 0);
    CHECK(t->root->height == 1);

    Vector* pl = avlSearch(t, "python");
    CHECK(pl != NULL);
    CHECK(pl->size == 1);

    PostingEntry* e = getVectorItem(pl, 0);
    CHECK(e->doc_id == 1);
    CHECK(strcmp(e->title, "How to use Python?") == 0);

    freeAVLTree(t);
}

/* ──────────────────────────────────────────────
   ТЕСТ 3: Дублирующийся ключ → дополнение posting list
   ────────────────────────────────────────────── */
static void test_duplicate_key(void) {
    printf("\n=== test_duplicate_key ===\n");
    AVLTree* t = createAVLTree();

    avlInsert(t, "sort", 10, "Sorting algorithms");
    avlInsert(t, "sort", 20, "Python sort() explained");
    avlInsert(t, "sort", 30, "Merge sort vs quicksort");

    /* Размер дерева не меняется при дублировании ключа */
    CHECK(t->size == 1);

    Vector* pl = avlSearch(t, "sort");
    CHECK(pl != NULL);
    CHECK((int)pl->size == 3);

    PostingEntry* e0 = getVectorItem(pl, 0);
    PostingEntry* e1 = getVectorItem(pl, 1);
    PostingEntry* e2 = getVectorItem(pl, 2);
    CHECK(e0->doc_id == 10);
    CHECK(e1->doc_id == 20);
    CHECK(e2->doc_id == 30);

    freeAVLTree(t);
}

/* ──────────────────────────────────────────────
   ТЕСТ 4: Поиск несуществующего ключа
   ────────────────────────────────────────────── */
static void test_search_missing(void) {
    printf("\n=== test_search_missing ===\n");
    AVLTree* t = createAVLTree();

    avlInsert(t, "avl", 1, "AVL trees");

    Vector* pl = avlSearch(t, "btree");
    CHECK(pl == NULL);

    Vector* pl2 = avlSearch(t, "");
    CHECK(pl2 == NULL);

    freeAVLTree(t);
}

/* ──────────────────────────────────────────────
   ТЕСТ 5: Правая ротация (Left-Left case)
   ────────────────────────────────────────────── */
static void test_rotate_right(void) {
    printf("\n=== test_rotate_right (LL case) ===\n");
    AVLTree* t = createAVLTree();

    /* Вставляем в убывающем порядке → LL-разбалансировка */
    avlInsert(t, "c", 3, "C");
    avlInsert(t, "b", 2, "B");
    avlInsert(t, "a", 1, "A");

    CHECK(check_avl_invariant(t->root) > 0);
    CHECK(check_bst_order(t->root, NULL, NULL));
    CHECK(t->size == 3);

    /* После ротации корень должен быть "b" */
    CHECK(strcmp(t->root->key, "b") == 0);

    freeAVLTree(t);
}

/* ──────────────────────────────────────────────
   ТЕСТ 6: Левая ротация (Right-Right case)
   ────────────────────────────────────────────── */
static void test_rotate_left(void) {
    printf("\n=== test_rotate_left (RR case) ===\n");
    AVLTree* t = createAVLTree();

    avlInsert(t, "a", 1, "A");
    avlInsert(t, "b", 2, "B");
    avlInsert(t, "c", 3, "C");

    CHECK(check_avl_invariant(t->root) > 0);
    CHECK(check_bst_order(t->root, NULL, NULL));
    CHECK(strcmp(t->root->key, "b") == 0);

    freeAVLTree(t);
}

/* ──────────────────────────────────────────────
   ТЕСТ 7: Left-Right ротация
   ────────────────────────────────────────────── */
static void test_rotate_lr(void) {
    printf("\n=== test_rotate_lr (LR case) ===\n");
    AVLTree* t = createAVLTree();

    avlInsert(t, "c", 3, "C");
    avlInsert(t, "a", 1, "A");
    avlInsert(t, "b", 2, "B");

    CHECK(check_avl_invariant(t->root) > 0);
    CHECK(check_bst_order(t->root, NULL, NULL));
    CHECK(strcmp(t->root->key, "b") == 0);

    freeAVLTree(t);
}

/* ──────────────────────────────────────────────
   ТЕСТ 8: Right-Left ротация
   ────────────────────────────────────────────── */
static void test_rotate_rl(void) {
    printf("\n=== test_rotate_rl (RL case) ===\n");
    AVLTree* t = createAVLTree();

    avlInsert(t, "a", 1, "A");
    avlInsert(t, "c", 3, "C");
    avlInsert(t, "b", 2, "B");

    CHECK(check_avl_invariant(t->root) > 0);
    CHECK(check_bst_order(t->root, NULL, NULL));
    CHECK(strcmp(t->root->key, "b") == 0);

    freeAVLTree(t);
}

/* ──────────────────────────────────────────────
   ТЕСТ 9: Массовая вставка — инвариант после каждой вставки
   ────────────────────────────────────────────── */
static void test_bulk_insert(void) {
    printf("\n=== test_bulk_insert ===\n");

    /* 20 лексикографически смешанных слов */
    const char* words[] = {
        "memory", "leak", "pointer", "null", "segfault",
        "stack", "heap", "overflow", "malloc", "free",
        "python", "list", "sort", "lambda", "iterator",
        "tree", "graph", "hash", "queue", "deque"
    };
    const int N = 20;

    AVLTree* t = createAVLTree();
    for (int i = 0; i < N; i++) {
        avlInsert(t, words[i], i + 1, words[i]);

        /* После каждой вставки проверяем оба инварианта */
        CHECK(check_avl_invariant(t->root) > 0);
        CHECK(check_bst_order(t->root, NULL, NULL));
    }

    CHECK(t->size == N);

    /* Поиск всех ключей */
    int all_found = 1;
    for (int i = 0; i < N; i++) {
        if (!avlSearch(t, words[i])) { all_found = 0; break; }
    }
    CHECK(all_found);

    freeAVLTree(t);
}

/* ──────────────────────────────────────────────
   ТЕСТ 10: In-order обход даёт лексикографический порядок
   ────────────────────────────────────────────── */

typedef struct { char prev[256]; int ordered; } OrderCtx;

static void order_visitor(const char* key, Vector* postings, void* ctx) {
    (void)postings;
    OrderCtx* c = ctx;
    if (c->prev[0] && strcmp(key, c->prev) <= 0) c->ordered = 0;
    strncpy(c->prev, key, 255);
    c->prev[255] = '\0';
}

static void test_inorder_traverse(void) {
    printf("\n=== test_inorder_traverse ===\n");

    const char* words[] = {
        "zebra", "mango", "apple", "cherry", "banana",
        "kiwi", "fig", "date", "elderberry", "grape"
    };
    const int N = 10;

    AVLTree* t = createAVLTree();
    for (int i = 0; i < N; i++)
        avlInsert(t, words[i], i + 1, words[i]);

    /* Счётчик */
    TraverseCtx tc = { .count = 0 };
    avlTraverse(t, count_visitor, &tc);
    CHECK(tc.count == N);

    /* Порядок */
    OrderCtx oc = { .prev = "", .ordered = 1 };
    avlTraverse(t, order_visitor, &oc);
    CHECK(oc.ordered == 1);

    freeAVLTree(t);
}

/* ──────────────────────────────────────────────
   ТЕСТ 11: freeAVLTree(NULL) не падает
   ────────────────────────────────────────────── */
static void test_free_null(void) {
    printf("\n=== test_free_null ===\n");
    freeAVLTree(NULL);
    CHECK(1); /* нет краша */
}

/* ──────────────────────────────────────────────
   ТЕСТ 12: Высота дерева логарифмическая
   ────────────────────────────────────────────── */
static void test_height_logarithmic(void) {
    printf("\n=== test_height_logarithmic ===\n");

    /* Вставляем 1000 слов вида "word_N" */
    AVLTree* t = createAVLTree();
    char buf[32];
    for (int i = 0; i < 1000; i++) {
        snprintf(buf, sizeof(buf), "word_%04d", i);
        avlInsert(t, buf, i, buf);
    }

    /* Для AVL-дерева высота ≤ 1.44 * log2(n+2) */
    int h = t->root ? t->root->height : 0;
    /* log2(1001) ≈ 10, ceiling = 1.44 * 10 ≈ 15 */
    CHECK(h <= 15);
    CHECK(t->size == 1000);

    freeAVLTree(t);
}

/* ──────────────────────────────────────────────
   main
   ────────────────────────────────────────────── */
int main(void) {
    printf("=== AVL Tree Tests ===\n");

    test_create_free();
    test_single_insert();
    test_duplicate_key();
    test_search_missing();
    test_rotate_right();
    test_rotate_left();
    test_rotate_lr();
    test_rotate_rl();
    test_bulk_insert();
    test_inorder_traverse();
    test_free_null();
    test_height_logarithmic();

    printf("\n==============================\n");
    printf("Result: %d/%d passed\n", g_passed, g_total);
    return (g_passed == g_total) ? EXIT_SUCCESS : EXIT_FAILURE;
}
