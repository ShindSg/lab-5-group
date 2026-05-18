#define _POSIX_C_SOURCE 200809L
#include "search.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define MAX_QUERY_TOKENS 64
#define TOP_K            10

// Пересечение posting list'ов (AND-семантика)

static int cmp_doc_id(const void* a, const void* b) {
    return ((PostingEntry*)a)->doc_id - ((PostingEntry*)b)->doc_id;
}

static int cmp_score_desc(const void* a, const void* b) {
    return ((SearchResult*)b)->score - ((SearchResult*)a)->score;
}

// Бинарный поиск doc_id в отсортированном массиве posting'ов
static PostingEntry* find_in_list(Vector* list, int doc_id) {
    size_t lo = 0, hi = list->size;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        PostingEntry* e = getVectorItem(list, mid);
        if      (e->doc_id == doc_id) return e;
        else if (e->doc_id  < doc_id) lo = mid + 1;
        else                          hi = mid;
    }
    return NULL;
}

Vector* intersectPostings(Vector** lists, int n) {
    Vector* result = createPostingList(); // временно — потом конвертируем

    if (n == 0) return result;

    // Сортируем каждый список по doc_id (копировать не нужно — они наши)
    for (int i = 0; i < n; i++)
        if (lists[i] && lists[i]->size > 0)
            qsort(lists[i]->data, lists[i]->size,
                  lists[i]->elem_size, cmp_doc_id);

    // Если хотя бы один список NULL — пересечение пусто
    for (int i = 0; i < n; i++)
        if (!lists[i]) return result;

    // Итерируем по самому короткому списку, ищем совпадения в остальных
    int shortest = 0;
    for (int i = 1; i < n; i++)
        if (lists[i]->size < lists[shortest]->size) shortest = i;

    Vector* out = createVector(sizeof(SearchResult));

    for (size_t j = 0; j < lists[shortest]->size; j++) {
        PostingEntry* base = getVectorItem(lists[shortest], j);
        int doc_id = base->doc_id;

        int found_in_all = 1;
        for (int i = 0; i < n; i++) {
            if (i == shortest) continue;
            if (!find_in_list(lists[i], doc_id)) {
                found_in_all = 0;
                break;
            }
        }

        if (found_in_all) {
            SearchResult sr;
            sr.doc_id = doc_id;
            sr.score  = n;
            strncpy(sr.title, base->title, MAX_TITLE_LEN - 1);
            sr.title[MAX_TITLE_LEN - 1] = '\0';
            appendVectorItem(out, &sr);
        }
    }

    vectorFree(result);
    return out;
}

// Токенизация запроса

static int tokenize_query(const char* query,
                           char tokens[][MAX_TITLE_LEN], int max_tokens) {
    char buf[4096];
    strncpy(buf, query, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    /* lower + замена не-словарных символов на пробел */
    for (char* p = buf; *p; p++) {
        *p = (char)tolower((unsigned char)*p);
        if (!isalnum((unsigned char)*p) && *p != '_')
            *p = ' ';
    }

    int n = 0;
    char* token = strtok(buf, " ");
    while (token && n < max_tokens) {
        if (strlen(token) > 2) {
            strncpy(tokens[n], token, MAX_TITLE_LEN - 1);
            tokens[n][MAX_TITLE_LEN - 1] = '\0';
            n++;
        }
        token = strtok(NULL, " ");
    }
    return n;
}

// Основной поиск

SearchResults* search(Index* idx, const char* query) {
    SearchResults* sr = malloc(sizeof(SearchResults));
    if (!sr) { perror("search"); exit(EXIT_FAILURE); }
    sr->results  = NULL;
    sr->total    = 0;
    sr->time_ms  = 0.0;

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    // Токенизируем запрос
    char tokens[MAX_QUERY_TOKENS][MAX_TITLE_LEN];
    int  n = tokenize_query(query, tokens, MAX_QUERY_TOKENS);

    if (n == 0) {
        sr->results = createVector(sizeof(SearchResult));
        return sr;
    }

    // Получаем posting list для каждого терма
    Vector* lists[MAX_QUERY_TOKENS];
    for (int i = 0; i < n; i++)
        lists[i] = lookupTerm(idx, tokens[i]);

    // Пересекаем
    Vector* all = intersectPostings(lists, n);

    sr->total = (int)all->size;

    // Берём топ-K по score (здесь score одинаковый, порядок — по doc_id)
    int top = sr->total < TOP_K ? sr->total : TOP_K;
    sr->results = createVector(sizeof(SearchResult));
    for (int i = 0; i < top; i++)
        appendVectorItem(sr->results, getVectorItem(all, i));

    vectorFree(all);

    clock_gettime(CLOCK_MONOTONIC, &t1);
    sr->time_ms = (t1.tv_sec  - t0.tv_sec)  * 1000.0
                + (t1.tv_nsec - t0.tv_nsec) / 1e6;

    return sr;
}

// Вывод результатов

void printResultsText(const SearchResults* sr) {
    if (!sr) return;
    printf("Время: %.1f мс | Найдено: %d документов\n\n",
           sr->time_ms, sr->total);

    if (!sr->results || sr->results->size == 0) {
        printf("Ничего не найдено.\n");
        return;
    }

    for (size_t i = 0; i < sr->results->size; i++) {
        SearchResult* r = getVectorItem(sr->results, i);
        printf("%2zu. [id=%d] %s\n", i + 1, r->doc_id, r->title);
    }
}

void printResultsJSON(const SearchResults* sr) {
    if (!sr) return;
    printf("{\n");
    printf("  \"time_ms\": %.3f,\n", sr->time_ms);
    printf("  \"total\": %d,\n", sr->total);
    printf("  \"results\": [\n");

    if (sr->results) {
        for (size_t i = 0; i < sr->results->size; i++) {
            SearchResult* r = getVectorItem(sr->results, i);

            // Экранируем кавычки в заголовке для валидного JSON
            char escaped[MAX_TITLE_LEN * 2];
            size_t ei = 0;
            for (const char* p = r->title; *p && ei < sizeof(escaped) - 2; p++) {
                if (*p == '"' || *p == '\\') escaped[ei++] = '\\';
                escaped[ei++] = *p;
            }
            escaped[ei] = '\0';

            printf("    {\"doc_id\": %d, \"title\": \"%s\", \"score\": %d}%s\n",
                   r->doc_id, escaped, r->score,
                   i + 1 < sr->results->size ? "," : "");
        }
    }

    printf("  ]\n}\n");
}

// Освобождение

void freeSearchResults(SearchResults* sr) {
    if (!sr) return;
    if (sr->results) vectorFree(sr->results);
    free(sr);
}


void freeFuzzyCandidates(Vector* candidates) {
    if (!candidates) return;
    for (size_t i = 0; i < candidates->size; i++) {
        FuzzyCandidate* c = getVectorItem(candidates, i);
        if (c->postings) vectorFree(c->postings);
    }
    vectorFree(candidates);
}

void fuzzyAppendCandidate(const char* term, Vector* postings, FuzzyContext* ctx) {
    if (!ctx || !ctx->candidates || !postings) return;
    int dist = levenshteinDistance(ctx->term, term);
    if (dist > ctx->maxdistance) return;

    FuzzyCandidate item;
    strncpy(item.term, term, sizeof(item.term) - 1);
    item.term[sizeof(item.term) - 1] = '\0';
    item.distance = dist;
    item.postings = clonePostingList(postings);
    appendVectorItem(ctx->candidates, &item);
}

Vector* fuzzyFindCandidates(Index* idx, const char* term, int max_distance) {
    if (!idx) return NULL;

    FuzzyContext* ctx = malloc(sizeof(FuzzyContext));
    if (!ctx) { perror("fuzzyFindCandidates"); exit(EXIT_FAILURE); }
    ctx->candidates  = createVector(sizeof(FuzzyCandidate));
    ctx->maxdistance = max_distance;
    strncpy(ctx->term, term, sizeof(ctx->term) - 1);
    ctx->term[sizeof(ctx->term) - 1] = '\0';

    traverseIndex(idx, (void (*)(const char*, Vector*, void*))fuzzyAppendCandidate, ctx);
    Vector* result = ctx->candidates;
    free(ctx);
    return result;
}

static int cmp_fuzzy_merged(const void* a, const void* b) {
    return ((FuzzyMergedEntry*)a)->doc_id - ((FuzzyMergedEntry*)b)->doc_id;
}

static Vector* mergeTokenCandidates(Vector* candidates) {
    // candidates: Vector<FuzzyCandidate>
    // Результат: Vector<FuzzyMergedEntry>
    Vector* merged = createVector(sizeof(FuzzyMergedEntry));

    for (size_t ci = 0; ci < candidates->size; ci++) {
        FuzzyCandidate* cand = getVectorItem(candidates, ci);
        Vector* postings = cand->postings;
        if (!postings) continue;

        for (size_t pi = 0; pi < postings->size; pi++) {
            PostingEntry* pe = getVectorItem(postings, pi);

            int found = 0;
            for (size_t mi = 0; mi < merged->size; mi++) {
                FuzzyMergedEntry* me = getVectorItem(merged, mi);
                if (me->doc_id == pe->doc_id) {
                    if (cand->distance < me->min_dist)
                        me->min_dist = cand->distance;
                    found = 1;
                    break;
                }
            }
            if (!found) {
                FuzzyMergedEntry me;
                me.doc_id   = pe->doc_id;
                me.min_dist = cand->distance;
                strncpy(me.title, pe->title, MAX_TITLE_LEN - 1);
                me.title[MAX_TITLE_LEN - 1] = '\0';
                appendVectorItem(merged, &me);
            }
        }
    }

    if (merged->size > 0)
        qsort(merged->data, merged->size, merged->elem_size, cmp_fuzzy_merged);

    return merged;
}

static FuzzyMergedEntry* find_merged(Vector* list, int doc_id) {
    size_t lo = 0, hi = list->size;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        FuzzyMergedEntry* e = getVectorItem(list, mid);
        if      (e->doc_id == doc_id) return e;
        else if (e->doc_id  < doc_id) lo = mid + 1;
        else                          hi = mid;
    }
    return NULL;
}

SearchResults* fuzzySearch(Index* idx, const char* query, int max_distance) {
    SearchResults* sr = malloc(sizeof(SearchResults));
    if (!sr) { perror("fuzzySearch"); exit(EXIT_FAILURE); }
    sr->results = NULL;
    sr->total   = 0;
    sr->time_ms = 0.0;

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    char tokens[MAX_QUERY_TOKENS][MAX_TITLE_LEN];
    int  n = tokenize_query(query, tokens, MAX_QUERY_TOKENS);

    if (n == 0) {
        sr->results = createVector(sizeof(SearchResult));
        clock_gettime(CLOCK_MONOTONIC, &t1);
        sr->time_ms = (t1.tv_sec - t0.tv_sec) * 1000.0
                    + (t1.tv_nsec - t0.tv_nsec) / 1e6;
        return sr;
    }

    Vector* token_lists[MAX_QUERY_TOKENS] = {NULL};
    int actual = 0;

    for (int i = 0; i < n; i++) {
        Vector* candidates = fuzzyFindCandidates(idx, tokens[i], max_distance);
        if (!candidates || candidates->size == 0) {
            if (candidates) freeFuzzyCandidates(candidates);

            for (int j = 0; j < actual; j++) vectorFree(token_lists[j]);
            sr->results = createVector(sizeof(SearchResult));
            clock_gettime(CLOCK_MONOTONIC, &t1);
            sr->time_ms = (t1.tv_sec - t0.tv_sec) * 1000.0
                        + (t1.tv_nsec - t0.tv_nsec) / 1e6;
            return sr;
        }

        token_lists[actual++] = mergeTokenCandidates(candidates);
        freeFuzzyCandidates(candidates);
    }

    int shortest = 0;
    for (int i = 1; i < actual; i++)
        if (token_lists[i]->size < token_lists[shortest]->size)
            shortest = i;

    Vector* out = createVector(sizeof(SearchResult));

    for (size_t j = 0; j < token_lists[shortest]->size; j++) {
        FuzzyMergedEntry* base = getVectorItem(token_lists[shortest], j);
        int doc_id = base->doc_id;

        int   found_in_all = 1;
        int   total_dist   = base->min_dist;

        for (int i = 0; i < actual; i++) {
            if (i == shortest) continue;
            FuzzyMergedEntry* hit = find_merged(token_lists[i], doc_id);
            if (!hit) { found_in_all = 0; break; }
            total_dist += hit->min_dist;
        }

        if (found_in_all) {
            double avg_dist = (double)total_dist / actual;
            SearchResult sr_item;
            sr_item.doc_id = doc_id;
            // score = matched_terms * 10 - avg_distance
            sr_item.score  = actual * 10 - (int)avg_dist;
            strncpy(sr_item.title, base->title, MAX_TITLE_LEN - 1);
            sr_item.title[MAX_TITLE_LEN - 1] = '\0';
            appendVectorItem(out, &sr_item);
        }
    }

    for (int i = 0; i < actual; i++) vectorFree(token_lists[i]);

    // сортировка по убыванию score
    if (out->size > 0) {
        qsort(out->data, out->size, out->elem_size, cmp_score_desc);
    }

    sr->total = (int)out->size;
    int top = sr->total < TOP_K ? sr->total : TOP_K;
    sr->results = createVector(sizeof(SearchResult));
    for (int i = 0; i < top; i++)
        appendVectorItem(sr->results, getVectorItem(out, i));
    vectorFree(out);

    clock_gettime(CLOCK_MONOTONIC, &t1);
    sr->time_ms = (t1.tv_sec - t0.tv_sec) * 1000.0
                + (t1.tv_nsec - t0.tv_nsec) / 1e6;
    return sr;
}