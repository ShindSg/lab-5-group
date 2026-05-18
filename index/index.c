#include "index.h"
#include "../avl/avl.h"
#include "../rbtree/rbtree.h"
#include "../btree/btree.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// Tree-specific index backend selection

Index* createIndex(TreeType type) {
    Index* idx = malloc(sizeof(Index));
    if (!idx) { perror("createIndex"); exit(EXIT_FAILURE); }

    idx->type = type;

    /*
     * Create concrete tree implementation
     * depending on selected TreeType.
     */
    switch (type) {
        case TREE_AVL:
            idx->tree = createAVLTree();
            break;

        case TREE_RB:
            idx->tree = createRBTree();
            break;

        case TREE_BTREE:
            idx->tree = createBTree();
            break;

        default:
            fprintf(stderr, "createIndex: unknown TreeType %d\n", type);
            exit(EXIT_FAILURE);
    }

    return idx;
}

void freeIndex(Index* idx) {
    if (!idx) return;

    /*
     * Free concrete tree implementation
     * according to stored tree type.
     */
    switch (idx->type) {
        case TREE_AVL:
            freeAVLTree(idx->tree);
            break;

        case TREE_RB:
            freeRBTree(idx->tree);
            break;

        case TREE_BTREE:
            freeBTree(idx->tree);
            break;
    }

    free(idx);
}


// Term insertion

void insertTerm(Index* idx, const char* term, int doc_id, const char* title) {
    if (!idx || !term) return;

    /*
     * Dispatch insertion to selected
     * tree implementation.
     */
    switch (idx->type) {
        case TREE_AVL:
            avlInsert(idx->tree, term, doc_id, title);
            break;

        case TREE_RB:
            rbInsert(idx->tree, term, doc_id, title);
            break;

        case TREE_BTREE:
            btreeInsert(idx->tree, term, doc_id, title);
            break;
    }
}

void indexDocument(Index* idx, int doc_id, const char* title,
                   const char** tokens, int n_tokens) {

    /*
     * Insert every token from document
     * into the index structure.
     */
    for (int i = 0; i < n_tokens; i++)
        insertTerm(idx, tokens[i], doc_id, title);
}


// Term lookup

Vector* lookupTerm(const Index* idx, const char* term) {
    if (!idx || !term) return NULL;

    /*
     * Dispatch lookup to selected
     * tree implementation.
     */
    switch (idx->type) {
        case TREE_AVL:
            return avlSearch(idx->tree, term);

        case TREE_RB:
            return rbSearch(idx->tree, term);

        case TREE_BTREE:
            return btreeSearch(idx->tree, term);
    }

    return NULL;
}


// Tree traversal

void traverseIndex(const Index* idx,
                   void (*visit)(const char* key, Vector* postings, void* ctx),
                   void* ctx) {

    if (!idx) return;

    /*
     * Traverse underlying tree
     * in sorted key order.
     */
    switch (idx->type) {
        case TREE_AVL:
            avlTraverse(idx->tree, visit, ctx);
            break;

        case TREE_RB:
            rbTraverse(idx->tree, visit, ctx);
            break;

        case TREE_BTREE:
            btreeTraverse(idx->tree, visit, ctx);
            break;
    }
}


// Index serialization

/*
 * Context passed into traversal callback
 * during index serialization.
 */
typedef struct {
    FILE* f;
    int   error;
} SaveCtx;

/*
 * Traversal callback used for saving
 * index entries into text file.
 */
static void save_visitor(const char* key, Vector* postings, void* ctx) {
    SaveCtx* sc = ctx;

    if (sc->error) return;

    /*
     * Save term and posting count:
     *
     *     term posting_count
     */
    if (fprintf(sc->f, "%s %zu\n", key, postings->size) < 0) {
        sc->error = 1;
        return;
    }

    /*
     * Save all posting entries:
     *
     *     doc_id title
     */
    for (size_t i = 0; i < postings->size; i++) {
        PostingEntry* e = getVectorItem(postings, i);

        /*
         * Replace possible newline characters
         * inside title with spaces to keep
         * one posting per line.
         */
        char safe_title[MAX_TITLE_LEN];

        strncpy(safe_title, e->title, MAX_TITLE_LEN - 1);
        safe_title[MAX_TITLE_LEN - 1] = '\0';

        for (char* p = safe_title; *p; p++)
            if (*p == '\n' || *p == '\r')
                *p = ' ';

        if (fprintf(sc->f, "%d %s\n", e->doc_id, safe_title) < 0) {
            sc->error = 1;
            return;
        }
    }
}

void saveIndex(const Index* idx, const char* path) {
    if (!idx || !path) return;

    FILE* f = fopen(path, "w");
    if (!f) {
        perror(path);
        return;
    }

    /*
     * Traverse index and serialize
     * all terms into file.
     */
    SaveCtx sc = { .f = f, .error = 0 };

    traverseIndex(idx, save_visitor, &sc);

    if (sc.error)
        fprintf(stderr, "saveIndex: write error on %s\n", path);

    fclose(f);
}


// Index deserialization

Index* loadIndex(const char* path, TreeType type) {
    if (!path) return NULL;

    FILE* f = fopen(path, "r");
    if (!f) {
        perror(path);
        return NULL;
    }

    Index* idx = createIndex(type);

    char term[MAX_TITLE_LEN];
    int  n_postings;

    /*
     * Buffer for:
     *
     *     doc_id + space + title
     */
    char line[MAX_TITLE_LEN + 32];

    /*
     * Read serialized format:
     *
     *     term posting_count
     */
    while (fscanf(f, "%s %d\n", term, &n_postings) == 2) {

        /*
         * Read all posting entries
         * for current term.
         */
        for (int i = 0; i < n_postings; i++) {

            if (!fgets(line, sizeof(line), f)) {
                fprintf(stderr, "loadIndex: unexpected EOF in %s\n", path);
                goto done;
            }

            /* Remove trailing newline */
            line[strcspn(line, "\n")] = '\0';

            /*
             * Split line:
             *
             *     doc_id title
             */
            char* space = strchr(line, ' ');

            if (!space) {
                fprintf(stderr, "loadIndex: malformed line: %s\n", line);
                continue;
            }

            *space = '\0';

            int         doc_id = atoi(line);
            const char* title  = space + 1;

            insertTerm(idx, term, doc_id, title);
        }
    }

done:
    fclose(f);

    return idx;
}
