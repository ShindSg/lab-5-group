#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "index/index.h"
#include "index/search.h"

#define _POSIX_C_SOURCE

/*
 * Maximum number of tokens parsed from one document.
 */
#define MAX_TOKENS_PER_DOC 4096

/*
 * Maximum token length during JSONL parsing.
 */
#define MAX_TOKEN_LEN      256


// Tree type helpers

/*
 * Converts string representation into TreeType.
 *
 * Supported values:
 * - "avl"
 * - "rb"
 * - "btree"
 */
TreeType parseType(const char* s) {
    if (strcmp(s, "rb") == 0)
        return TREE_RB;

    if (strcmp(s, "btree") == 0)
        return TREE_BTREE;

    /* Default backend */
    return TREE_AVL;
}

/*
 * Converts TreeType into printable string name.
 */
const char* typeName(TreeType type) {
    switch (type) {
        case TREE_RB:
            return "rb";

        case TREE_BTREE:
            return "btree";

        default:
            return "avl";
    }
}


// JSONL parsing

/*
 * Parses a single JSONL document line.
 *
 * Extracts:
 * - document ID;
 * - document title;
 * - token list.
 *
 * Returns:
 *   1 on success;
 *   0 on malformed input.
 */
static int parse_jsonl_line(const char* line,
                             int*   doc_id,
                             char*  title,
                             size_t title_cap,
                             char** tokens,
                             int*   n_tokens) {
    // Parse doc_id

    const char* p = strstr(line, "\"doc_id\"");
    if (!p) return 0;

    p = strchr(p, ':');
    if (!p) return 0;

    /*
     * doc_id may appear either as:
     * - "80"
     * - 80
     */
    while (*p == ':' || *p == ' ' || *p == '"')
        p++;

    *doc_id = atoi(p);

    // Parse title

    p = strstr(line, "\"title\"");
    if (!p) return 0;

    p = strchr(p, ':');
    if (!p) return 0;

    p++;

    while (*p == ' ')
        p++;

    if (*p != '"')
        return 0;

    /* Skip opening quote */
    p++;

    size_t ti = 0;

    while (*p && *p != '"' && ti < title_cap - 1) {
        /*
         * Handle basic JSON escape sequences.
         */
        if (*p == '\\' && *(p + 1)) {
            p++;

            switch (*p) {
                case 'n':
                    title[ti++] = '\n';
                    break;

                case 't':
                    title[ti++] = '\t';
                    break;

                case '"':
                    title[ti++] = '"';
                    break;

                case '\\':
                    title[ti++] = '\\';
                    break;

                default:
                    title[ti++] = *p;
                    break;
            }
        } else {
            title[ti++] = *p;
        }

        p++;
    }

    title[ti] = '\0';

    // Parse tokens array

    p = strstr(line, "\"tokens\"");
    if (!p) return 0;

    p = strchr(p, '[');
    if (!p) return 0;

    p++;

    *n_tokens = 0;

    while (*p && *p != ']') {
        while (*p == ' ' || *p == ',')
            p++;

        if (*p != '"') {
            p++;
            continue;
        }

        /* Skip opening quote */
        p++;

        int ti2 = 0;

        while (*p && *p != '"' && ti2 < MAX_TOKEN_LEN - 1)
            tokens[*n_tokens][ti2++] = *p++;

        tokens[*n_tokens][ti2] = '\0';

        if (ti2 > 0)
            (*n_tokens)++;

        /* Skip closing quote */
        if (*p == '"')
            p++;
    }

    return 1;
}


// Index building

/*
 * Builds index from JSONL dataset and saves it to disk.
 */
static void runIndex(TreeType type,
                     const char* data_path,
                     const char* idx_path) {
    FILE* f = fopen(data_path, "r");

    if (!f) {
        fprintf(stderr, "Failed to open %s\n", data_path);
        exit(EXIT_FAILURE);
    }

    Index* idx = createIndex(type);

    /*
     * Buffers used during JSONL parsing.
     */
    char  line[1024 * 64];
    char  title[MAX_TITLE_LEN];

    char  token_storage[MAX_TOKENS_PER_DOC][MAX_TOKEN_LEN];
    char* token_ptrs[MAX_TOKENS_PER_DOC];

    for (int i = 0; i < MAX_TOKENS_PER_DOC; i++)
        token_ptrs[i] = token_storage[i];

    int doc_id;
    int n_tokens;

    long docs = 0;

    struct timespec t0, t1;

    clock_gettime(CLOCK_MONOTONIC, &t0);

    while (fgets(line, sizeof(line), f)) {
        /* Skip empty lines */
        if (line[0] == '\n' || line[0] == '\0')
            continue;

        if (!parse_jsonl_line(
                line,
                &doc_id,
                title,
                sizeof(title),
                token_ptrs,
                &n_tokens))
            continue;

        /*
         * Insert document tokens into selected backend.
         */
        indexDocument(
            idx,
            doc_id,
            title,
            (const char**)token_ptrs,
            n_tokens
        );

        docs++;

        /*
         * Print progress every 10k documents.
         */
        if (docs % 10000 == 0)
            fprintf(stderr,
                    "Indexed: %ld documents\n",
                    docs);
    }

    fclose(f);

    clock_gettime(CLOCK_MONOTONIC, &t1);

    double elapsed =
        (t1.tv_sec  - t0.tv_sec)  * 1000.0 +
        (t1.tv_nsec - t0.tv_nsec) / 1e6;

    fprintf(stderr,
            "Done: %ld documents in %.1f ms\n",
            docs,
            elapsed);

    saveIndex(idx, idx_path);

    fprintf(stderr,
            "Index saved: %s\n",
            idx_path);

    freeIndex(idx);
}


/*
 * Loads existing index and executes query search.
 */
static void runSearch(TreeType type,
                      const char* idx_path,
                      const char* query,
                      int json_out) {
    Index* idx = loadIndex(idx_path, type);

    if (!idx) {
        fprintf(stderr,
                "Failed to load index: %s\n",
                idx_path);

        exit(EXIT_FAILURE);
    }

    SearchResults* sr = search(idx, query);

    if (json_out)
        printResultsJSON(sr);
    else
        printResultsText(sr);

    freeSearchResults(sr);
    freeIndex(idx);
}


/*
 * Prints CLI usage information.
 */
static void usage(const char* prog) {
    fprintf(
        stderr,
        "Usage:\n"
        "  %s index  --type=<avl|rb|btree> "
        "[--data=PATH] [--index=PATH]\n"
        "  %s search --type=<avl|rb|btree> "
        "[--index=PATH] [--json] \"query\"\n",
        prog,
        prog
    );
}


// Main CLI entry point

int main(int argc, char* argv[]) {
    if (argc < 3) {
        usage(argv[0]);
        return 1;
    }

    const char* mode = argv[1];

    TreeType type = TREE_AVL;

    const char* data_path =
        "data/processed/docs.jsonl";

    char idx_path[512] = {0};

    int json_out = 0;

    const char* query = NULL;

    /*
     * Parse command-line arguments.
     */
    for (int i = 2; i < argc; i++) {
        if (strncmp(argv[i], "--type=", 7) == 0) {
            type = parseType(argv[i] + 7);
        }

        else if (strncmp(argv[i], "--data=", 7) == 0) {
            data_path = argv[i] + 7;
        }

        else if (strncmp(argv[i], "--index=", 8) == 0) {
            strncpy(
                idx_path,
                argv[i] + 8,
                sizeof(idx_path) - 1
            );
        }

        else if (strcmp(argv[i], "--json") == 0) {
            json_out = 1;
        }

        /*
         * Non-option argument is treated as query string.
         */
        else if (argv[i][0] != '-') {
            query = argv[i];
        }
    }

    /*
     * Generate default index path if not specified.
     */
    if (idx_path[0] == '\0') {
        snprintf(
            idx_path,
            sizeof(idx_path),
            "data/index_%s.txt",
            typeName(type)
        );
    }

    // Indexing mode

    if (strcmp(mode, "index") == 0) {
        runIndex(type, data_path, idx_path);
    }

    // Search mode

    else if (strcmp(mode, "search") == 0) {
        if (!query) {
            fprintf(stderr, "No query provided\n");
            return 1;
        }

        runSearch(type, idx_path, query, json_out);
    }

    // Unknown mode

    else {
        fprintf(stderr,
                "Unknown mode: %s\n",
                mode);

        usage(argv[0]);

        return 1;
    }

    return 0;
}
