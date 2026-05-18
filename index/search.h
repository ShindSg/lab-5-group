#pragma once

#include "index.h"

typedef struct {
    int  doc_id;
    char title[MAX_TITLE_LEN];
    int  score;
} SearchResult;

typedef struct {
    Vector* results;   /* elements: SearchResult, топ-10 */
    int     total;     /* всего найдено документов       */
    double  time_ms;
} SearchResults;

typedef struct {
    char    term[256];
    int     distance;
    Vector* postings;
} FuzzyCandidate;

typedef struct
{
    char     term[256];
    int    maxdistance;
    Vector *candidates;
} FuzzyContext;

typedef struct {
    int doc_id;
    char title[MAX_TITLE_LEN];
    int min_dist;
} FuzzyMergedEntry;


Vector*        intersectPostings(Vector** lists, int n);
SearchResults* search(Index* idx, const char* query);
void           printResultsText(const SearchResults* sr);
void           printResultsJSON(const SearchResults* sr);
void           freeSearchResults(SearchResults* sr);

/* Список кандидатов — Vector с элементами FuzzyCandidate */
Vector* fuzzyFindCandidates(Index* idx, const char* term, int max_distance);

/* Нечёткий поиск с ранжированием */
SearchResults* fuzzySearch(Index* idx, const char* query, int max_distance);

void freeFuzzyCandidates(Vector* candidates);