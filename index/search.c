#include "search.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define _POSIX_C_SOURCE
#define MAX_QUERY_TOKENS 64
#define TOP_K            10


// Posting list intersection (AND semantics)

/*
 * Comparator for sorting posting entries by doc_id.
 */
static int cmp_doc_id(const void* a, const void* b) {
    return ((PostingEntry*)a)->doc_id - ((PostingEntry*)b)->doc_id;
}

/*
 * Binary search for doc_id inside sorted posting list.
 *
 * Returns:
 * - pointer to PostingEntry if found;
 * - NULL otherwise.
 */
static PostingEntry* find_in_list(Vector* list, int doc_id) {
    size_t lo = 0;
    size_t hi = list->size;

    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;

        PostingEntry* e = getVectorItem(list, mid);

        if      (e->doc_id == doc_id) return e;
        else if (e->doc_id  < doc_id) lo = mid + 1;
        else                          hi = mid;
    }

    return NULL;
}

/*
 * Computes intersection of multiple posting lists.
 *
 * Uses AND semantics:
 * document must appear in every posting list.
 */
Vector* intersectPostings(Vector** lists, int n) {
    /* Temporary vector used for empty-result handling */
    Vector* result = createPostingList();

    if (n == 0)
        return result;

    /*
     * Sort all posting lists by doc_id
     * to allow binary search.
     */
    for (int i = 0; i < n; i++) {
        if (lists[i] && lists[i]->size > 0) {
            qsort(
                lists[i]->data,
                lists[i]->size,
                lists[i]->elem_size,
                cmp_doc_id
            );
        }
    }

    /* If at least one posting list is NULL, intersection is empty */
    for (int i = 0; i < n; i++)
        if (!lists[i])
            return result;

    /*
     * Iterate over shortest posting list
     * to minimize number of checks.
     */
    int shortest = 0;

    for (int i = 1; i < n; i++)
        if (lists[i]->size < lists[shortest]->size)
            shortest = i;

    Vector* out = createVector(sizeof(SearchResult));

    for (size_t j = 0; j < lists[shortest]->size; j++) {
        PostingEntry* base = getVectorItem(lists[shortest], j);

        int doc_id = base->doc_id;

        int found_in_all = 1;

        /*
         * Check whether current document
         * exists in every posting list.
         */
        for (int i = 0; i < n; i++) {
            if (i == shortest)
                continue;

            if (!find_in_list(lists[i], doc_id)) {
                found_in_all = 0;
                break;
            }
        }

        /*
         * Add matching document
         * into final result set.
         */
        if (found_in_all) {
            SearchResult sr;

            sr.doc_id = doc_id;

            /*
             * Current score strategy:
             * score equals number of matched terms.
             */
            sr.score  = n;

            strncpy(sr.title, base->title, MAX_TITLE_LEN - 1);
            sr.title[MAX_TITLE_LEN - 1] = '\0';

            appendVectorItem(out, &sr);
        }
    }

    vectorFree(result);

    return out;
}


// Query tokenization

/*
 * Splits query string into normalized tokens.
 *
 * Processing steps:
 * - lowercase conversion;
 * - replacing non-alphanumeric characters with spaces;
 * - filtering very short tokens.
 *
 * Returns:
 *   Number of extracted tokens.
 */
static int tokenize_query(const char* query,
                           char tokens[][MAX_TITLE_LEN], int max_tokens) {

    char buf[4096];

    strncpy(buf, query, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    /*
     * Lowercase conversion and replacement
     * of non-word characters with spaces.
     */
    for (char* p = buf; *p; p++) {
        *p = (char)tolower((unsigned char)*p);

        if (!isalnum((unsigned char)*p) && *p != '_')
            *p = ' ';
    }

    int n = 0;

    char* token = strtok(buf, " ");

    while (token && n < max_tokens) {

        /* Ignore very short tokens */
        if (strlen(token) > 2) {
            strncpy(tokens[n], token, MAX_TITLE_LEN - 1);
            tokens[n][MAX_TITLE_LEN - 1] = '\0';

            n++;
        }

        token = strtok(NULL, " ");
    }

    return n;
}


// Main search pipeline

SearchResults* search(Index* idx, const char* query) {
    SearchResults* sr = malloc(sizeof(SearchResults));

    if (!sr) {
        perror("search");
        exit(EXIT_FAILURE);
    }

    sr->results  = NULL;
    sr->total    = 0;
    sr->time_ms  = 0.0;

    struct timespec t0, t1;

    clock_gettime(CLOCK_MONOTONIC, &t0);

    /* Tokenize input query */
    char tokens[MAX_QUERY_TOKENS][MAX_TITLE_LEN];

    int n = tokenize_query(query, tokens, MAX_QUERY_TOKENS);

    /*
     * Empty query produces empty result set.
     */
    if (n == 0) {
        sr->results = createVector(sizeof(SearchResult));
        return sr;
    }

    /*
     * Retrieve posting list
     * for every query token.
     */
    Vector* lists[MAX_QUERY_TOKENS];

    for (int i = 0; i < n; i++)
        lists[i] = lookupTerm(idx, tokens[i]);

    /* Intersect posting lists */
    Vector* all = intersectPostings(lists, n);

    sr->total = (int)all->size;

    /*
     * Keep only top-K results.
     *
     * Current ordering:
     * - score;
     * - then doc_id.
     */
    int top = sr->total < TOP_K ? sr->total : TOP_K;

    sr->results = createVector(sizeof(SearchResult));

    for (int i = 0; i < top; i++)
        appendVectorItem(sr->results, getVectorItem(all, i));

    vectorFree(all);

    clock_gettime(CLOCK_MONOTONIC, &t1);

    /* Compute elapsed search time in milliseconds */
    sr->time_ms =
        (t1.tv_sec  - t0.tv_sec)  * 1000.0 +
        (t1.tv_nsec - t0.tv_nsec) / 1e6;

    return sr;
}


// Result printing

/*
 * Prints search results in plain text format.
 */
void printResultsText(const SearchResults* sr) {
    if (!sr) return;

    printf(
        "Время: %.1f мс | Найдено: %d документов\n\n",
        sr->time_ms,
        sr->total
    );

    if (!sr->results || sr->results->size == 0) {
        printf("Ничего не найдено.\n");
        return;
    }

    for (size_t i = 0; i < sr->results->size; i++) {
        SearchResult* r = getVectorItem(sr->results, i);

        printf("%2zu. [id=%d] %s\n",
               i + 1,
               r->doc_id,
               r->title);
    }
}

/*
 * Prints search results in JSON format.
 */
void printResultsJSON(const SearchResults* sr) {
    if (!sr) return;

    printf("{\n");
    printf("  \"time_ms\": %.3f,\n", sr->time_ms);
    printf("  \"total\": %d,\n", sr->total);
    printf("  \"results\": [\n");

    if (sr->results) {
        for (size_t i = 0; i < sr->results->size; i++) {

            SearchResult* r = getVectorItem(sr->results, i);

            /*
             * Escape quotes and backslashes
             * to produce valid JSON.
             */
            char escaped[MAX_TITLE_LEN * 2];

            size_t ei = 0;

            for (const char* p = r->title;
                 *p && ei < sizeof(escaped) - 2;
                 p++) {

                if (*p == '"' || *p == '\\')
                    escaped[ei++] = '\\';

                escaped[ei++] = *p;
            }

            escaped[ei] = '\0';

            printf(
                "    {\"doc_id\": %d, \"title\": \"%s\", \"score\": %d}%s\n",
                r->doc_id,
                escaped,
                r->score,
                i + 1 < sr->results->size ? "," : ""
            );
        }
    }

    printf("  ]\n}\n");
}


// Search result cleanup

/*
 * Frees SearchResults structure.
 */
void freeSearchResults(SearchResults* sr) {
    if (!sr) return;

    if (sr->results)
        vectorFree(sr->results);

    free(sr);
}
