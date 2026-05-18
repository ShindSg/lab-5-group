#pragma once

#include "index.h"

/*
 * Single search result entry.
 *
 * doc_id identifies matching document.
 * title stores document title.
 * score stores current ranking score.
 */
typedef struct {
    int  doc_id;
    char title[MAX_TITLE_LEN];
    int  score;
} SearchResult;

/*
 * Search result container.
 *
 * results stores SearchResult entries.
 * total stores total number of matched documents.
 * time_ms stores elapsed search time in milliseconds.
 */
typedef struct {
    Vector* results;   /* elements: SearchResult, top-10 */
    int     total;     /* total number of matched documents */
    double  time_ms;
} SearchResults;

/*
 * Computes intersection of multiple posting lists.
 *
 * Uses AND semantics:
 * document must appear in every posting list.
 *
 * Returns:
 *   Vector containing SearchResult elements.
 */
Vector* intersectPostings(Vector** lists, int n);

/*
 * Executes search query against index.
 *
 * Query is tokenized and normalized before lookup.
 *
 * Returns:
 *   Newly allocated SearchResults structure.
 *
 * Returned value must be freed with freeSearchResults().
 */
SearchResults* search(Index* idx, const char* query);

/*
 * Prints search results in human-readable text format.
 */
void printResultsText(const SearchResults* sr);

/*
 * Prints search results in JSON format.
 */
void printResultsJSON(const SearchResults* sr);

/*
 * Frees SearchResults structure.
 *
 * Passing NULL is allowed.
 */
void freeSearchResults(SearchResults* sr);
