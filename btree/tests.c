#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "btree.h"
#include "../posting.h"



/*
 * Context used for traversal tests.
 * Stores visited keys in traversal order.
 */
typedef struct {
    const char* keys[128];
    int count;
} TraverseCtx;


/*
 * Traversal callback that collects keys.
 */
static void collect_key(const char* key, Vector* postings, void* ctx)
{
    (void)postings;

    TraverseCtx* c = ctx;
    c->keys[c->count++] = key;
}

/*
 * Convenience wrapper around getVectorItem().
 * Returns posting entry at the specified index.
 */
static PostingEntry* get_posting(Vector* postings, size_t idx)
{
    return (PostingEntry*)getVectorItem(postings, idx);
}


/*==========================================================
 * B-tree creation tests
 *==========================================================*/


static void test_create_btree(void)
{
    BTree* tree = createBTree();

    assert(tree != NULL);
    assert(tree->root != NULL);
    assert(tree->root->is_leaf == 1);
    assert(tree->root->n == 0);
    assert(tree->size == 0);

    freeBTree(tree);
}


/*==========================================================
 * Single insertion and search tests
 *==========================================================*/


static void test_insert_and_search_one_key(void)
{
    BTree* tree = createBTree();
    assert(tree != NULL);

    btreeInsert(tree, "apple", 1, "Doc 1");

    assert(tree->size == 1);

    Vector* postings = btreeSearch(tree, "apple");
    assert(postings != NULL);
    assert(postings->size == 1);

    PostingEntry* entry = get_posting(postings, 0);
    assert(entry != NULL);
    assert(entry->doc_id == 1);
    assert(strcmp(entry->title, "Doc 1") == 0);

    assert(btreeSearch(tree, "banana") == NULL);

    freeBTree(tree);
}


/*==========================================================
 * Duplicate key handling tests
 *==========================================================*/


static void test_duplicate_key_adds_posting(void)
{
    BTree* tree = createBTree();
    assert(tree != NULL);

    btreeInsert(tree, "apple", 1, "Doc 1");
    btreeInsert(tree, "apple", 2, "Doc 2");
    btreeInsert(tree, "apple", 3, "Doc 3");

    assert(tree->size == 1);

    Vector* postings = btreeSearch(tree, "apple");
    assert(postings != NULL);
    assert(postings->size == 3);

    PostingEntry* p0 = get_posting(postings, 0);
    PostingEntry* p1 = get_posting(postings, 1);
    PostingEntry* p2 = get_posting(postings, 2);

    assert(p0 && p1 && p2);
    assert(p0->doc_id == 1);
    assert(p1->doc_id == 2);
    assert(p2->doc_id == 3);

    assert(strcmp(p0->title, "Doc 1") == 0);
    assert(strcmp(p1->title, "Doc 2") == 0);
    assert(strcmp(p2->title, "Doc 3") == 0);

    freeBTree(tree);
}


/*==========================================================
 * Node split tests
 *==========================================================*/


static void test_insert_many_causes_split(void)
{
    BTree* tree = createBTree();
    assert(tree != NULL);

    btreeInsert(tree, "01", 1, "Doc 1");
    btreeInsert(tree, "02", 2, "Doc 2");
    btreeInsert(tree, "03", 3, "Doc 3");
    btreeInsert(tree, "04", 4, "Doc 4");
    btreeInsert(tree, "05", 5, "Doc 5");
    btreeInsert(tree, "06", 6, "Doc 6");
    btreeInsert(tree, "07", 7, "Doc 7");
    btreeInsert(tree, "08", 8, "Doc 8");
    btreeInsert(tree, "09", 9, "Doc 9");
    btreeInsert(tree, "10", 10, "Doc 10");

    assert(tree->size == 10);
    assert(tree->root != NULL);
    assert(tree->root->is_leaf == 0);

    for (int i = 1; i <= 10; i++) {
        char key[8];
        snprintf(key, sizeof(key), "%02d", i);

        Vector* postings = btreeSearch(tree, key);
        assert(postings != NULL);
        assert(postings->size == 1);

        PostingEntry* p = get_posting(postings, 0);
        assert(p != NULL);
        assert(p->doc_id == i);
    }

    assert(btreeSearch(tree, "11") == NULL);

    freeBTree(tree);
}


/*==========================================================
 * Traversal tests
 *==========================================================*/


static void test_traverse_sorted_order(void)
{
    BTree* tree = createBTree();
    assert(tree != NULL);

    btreeInsert(tree, "delta", 1, "Doc delta");
    btreeInsert(tree, "alpha", 2, "Doc alpha");
    btreeInsert(tree, "charlie", 3, "Doc charlie");
    btreeInsert(tree, "bravo", 4, "Doc bravo");
    btreeInsert(tree, "echo", 5, "Doc echo");
    btreeInsert(tree, "foxtrot", 6, "Doc foxtrot");
    btreeInsert(tree, "golf", 7, "Doc golf");

    TraverseCtx ctx;
    ctx.count = 0;

    btreeTraverse(tree, collect_key, &ctx);

    assert(ctx.count == 7);
    assert(strcmp(ctx.keys[0], "alpha") == 0);
    assert(strcmp(ctx.keys[1], "bravo") == 0);
    assert(strcmp(ctx.keys[2], "charlie") == 0);
    assert(strcmp(ctx.keys[3], "delta") == 0);
    assert(strcmp(ctx.keys[4], "echo") == 0);
    assert(strcmp(ctx.keys[5], "foxtrot") == 0);
    assert(strcmp(ctx.keys[6], "golf") == 0);

    freeBTree(tree);
}


/*==========================================================
 * Invalid argument handling tests
 *==========================================================*/


static void test_null_arguments(void)
{
    assert(btreeSearch(NULL, "abc") == NULL);

    BTree* tree = createBTree();
    assert(tree != NULL);

    assert(btreeSearch(tree, NULL) == NULL);

    btreeInsert(NULL, "abc", 1, "Doc");
    btreeInsert(tree, NULL, 1, "Doc");
    btreeInsert(tree, "abc", 1, NULL);

    assert(tree->size == 0);

    btreeTraverse(NULL, collect_key, NULL);
    btreeTraverse(tree, NULL, NULL);

    freeBTree(tree);
    freeBTree(NULL);
}


/*==========================================================
 * Test runner
 *==========================================================*/


int main(void)
{
    test_create_btree();
    test_insert_and_search_one_key();
    test_duplicate_key_adds_posting();
    test_insert_many_causes_split();
    test_traverse_sorted_order();
    test_null_arguments();

    printf("All B-tree unit tests passed.\n");
    return 0;
}
