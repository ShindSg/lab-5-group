/*
 * tests.c — unit tests for AVL tree implementation
 *
 * Build:
 *   gcc -Wall -Wextra -o test_avl tests.c avl.c ../posting.c \
 *   ../instruments/lab3/vector/generic.c -I.. ; ./test_avl
 *
 * Output:
 *   [PASS] / [FAIL] for each test
 *   Final score: N/M passed
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "avl.h"


static int g_passed = 0;
static int g_total  = 0;


/*
 * Simple assertion macro for test reporting.
 */
#define CHECK(cond) do {                                          \
    g_total++;                                                    \
    if (cond) { g_passed++; printf("[PASS] %s\n", #cond); }       \
    else       printf("[FAIL] %s  (line %d)\n", #cond, __LINE__); \
} while(0)


// ==========================================================
// Helper utilities
// ==========================================================

/*
 * Recursively checks AVL invariant.
 *
 * Returns:
 * - subtree height on success;
 * - -1 if AVL property is violated.
 */
static int check_avl_invariant(const AVLNode* n) {
    if (!n) return 0;

    int lh = check_avl_invariant(n->left);
    int rh = check_avl_invariant(n->right);

    if (lh < 0 || rh < 0)
        return -1;

    /* AVL balance factor must stay within [-1; 1] */
    int diff = lh - rh;

    if (diff < -1 || diff > 1)
        return -1;

    /* Stored height must match computed subtree height */
    int expected_height = 1 + (lh > rh ? lh : rh);

    if (n->height != expected_height)
        return -1;

    return expected_height;
}

/*
 * Recursively checks BST ordering property.
 */
static int check_bst_order(const AVLNode* n,
                            const char* lo, const char* hi) {
    if (!n) return 1;

    if (lo && strcmp(n->key, lo) <= 0)
        return 0;

    if (hi && strcmp(n->key, hi) >= 0)
        return 0;

    return check_bst_order(n->left,  lo, n->key)
        && check_bst_order(n->right, n->key, hi);
}

/*
 * Context used for counting visited nodes.
 */
typedef struct {
    int count;
} TraverseCtx;

/*
 * Traversal callback that counts visited nodes.
 */
static void count_visitor(const char* key, Vector* postings, void* ctx) {
    (void)key;
    (void)postings;

    ((TraverseCtx*)ctx)->count++;
}


// ==========================================================
// Test 1: Empty tree creation and cleanup
// ==========================================================

static void test_create_free(void) {
    printf("\n=== test_create_free ===\n");

    AVLTree* t = createAVLTree();

    CHECK(t != NULL);
    CHECK(t->root == NULL);
    CHECK(t->size == 0);

    freeAVLTree(t);

    int no_crash = 1;

    /* freeAVLTree completed without crash */
    CHECK(no_crash);
}


// ==========================================================
// Test 2: Single key insertion
// ==========================================================

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


// ==========================================================
// Test 3: Duplicate key appends posting
// ==========================================================

static void test_duplicate_key(void) {
    printf("\n=== test_duplicate_key ===\n");

    AVLTree* t = createAVLTree();

    avlInsert(t, "sort", 10, "Sorting algorithms");
    avlInsert(t, "sort", 20, "Python sort() explained");
    avlInsert(t, "sort", 30, "Merge sort vs quicksort");

    /* Tree size must not increase for duplicate keys */
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


// ==========================================================
// Test 4: Search for missing key
// ==========================================================

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


// ==========================================================
// Test 5: Right rotation (LL case)
// ==========================================================

static void test_rotate_right(void) {
    printf("\n=== test_rotate_right (LL case) ===\n");

    AVLTree* t = createAVLTree();

    /*
     * Descending insertion order produces
     * Left-Left imbalance.
     */
    avlInsert(t, "c", 3, "C");
    avlInsert(t, "b", 2, "B");
    avlInsert(t, "a", 1, "A");

    CHECK(check_avl_invariant(t->root) > 0);
    CHECK(check_bst_order(t->root, NULL, NULL));
    CHECK(t->size == 3);

    /* After rotation root must become "b" */
    CHECK(strcmp(t->root->key, "b") == 0);

    freeAVLTree(t);
}


// ==========================================================
// Test 6: Left rotation (RR case)
// ==========================================================

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


// ==========================================================
// Test 7: Left-Right rotation
// ==========================================================

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


// ==========================================================
// Test 8: Right-Left rotation
// ==========================================================

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


// ==========================================================
// Test 9: Bulk insertion
// ==========================================================

static void test_bulk_insert(void) {
    printf("\n=== test_bulk_insert ===\n");

    /*
     * Mixed lexicographical dataset.
     */
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

        /* Check invariants after every insertion */
        CHECK(check_avl_invariant(t->root) > 0);
        CHECK(check_bst_order(t->root, NULL, NULL));
    }

    CHECK(t->size == N);

    /* Verify that all inserted keys are searchable */
    int all_found = 1;

    for (int i = 0; i < N; i++) {
        if (!avlSearch(t, words[i])) {
            all_found = 0;
            break;
        }
    }

    CHECK(all_found);

    freeAVLTree(t);
}


// ==========================================================
// Test 10: In-order traversal ordering
// ==========================================================

typedef struct {
    char prev[256];
    int ordered;
} OrderCtx;

/*
 * Traversal callback used for ordering validation.
 */
static void order_visitor(const char* key, Vector* postings, void* ctx) {
    (void)postings;

    OrderCtx* c = ctx;

    if (c->prev[0] && strcmp(key, c->prev) <= 0)
        c->ordered = 0;

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

    /* Count visited nodes */
    TraverseCtx tc = { .count = 0 };

    avlTraverse(t, count_visitor, &tc);

    CHECK(tc.count == N);

    /* Verify lexicographical ordering */
    OrderCtx oc = { .prev = "", .ordered = 1 };

    avlTraverse(t, order_visitor, &oc);

    CHECK(oc.ordered == 1);

    freeAVLTree(t);
}


// ==========================================================
// Test 11: freeAVLTree(NULL)
// ==========================================================

static void test_free_null(void) {
    printf("\n=== test_free_null ===\n");

    freeAVLTree(NULL);

    int no_crash = 1;

    /* Function must safely handle NULL */
    CHECK(no_crash);
}


// ==========================================================
// Test 11b: NULL tree guards
// ==========================================================

static void test_null_tree_guards(void) {
    printf("\n=== test_null_tree_guards ===\n");

    Vector* pl = avlSearch(NULL, "key");

    CHECK(pl == NULL);

    int traverse_ok = 1;

    /* Traversal on NULL tree must not crash */
    avlTraverse(NULL, count_visitor, &traverse_ok);

    CHECK(traverse_ok);
}


// ==========================================================
// Test 12: Logarithmic tree height
// ==========================================================

static void test_height_logarithmic(void) {
    printf("\n=== test_height_logarithmic ===\n");

    const int N = 1000;

    AVLTree* t = createAVLTree();

    char buf[32];

    /*
     * Insert many sequential keys.
     * AVL balancing must keep logarithmic height.
     */
    for (int i = 0; i < N; i++) {
        snprintf(buf, sizeof(buf), "word_%04d", i);
        avlInsert(t, buf, i, buf);
    }

    /*
     * AVL upper bound:
     * height <= 1.44 * log2(n + 2)
     */
    int h = t->root ? t->root->height : 0;

    int max_height =
        (int)(1.44 * log2((double)(N + 2))) + 1;

    CHECK(h <= max_height);
    CHECK(t->size == N);

    freeAVLTree(t);
}


// ==========================================================
// Main test runner
// ==========================================================

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
    test_null_tree_guards();
    test_height_logarithmic();

    printf("\n==============================\n");
    printf("Result: %d/%d passed\n", g_passed, g_total);

    return (g_passed == g_total)
        ? EXIT_SUCCESS
        : EXIT_FAILURE;
}
