#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "index/index.h"
#include "index/search.h"


// Вспомогательные функции типа дерева

TreeType parseType(const char* s) {
    if (strcmp(s, "rb")    == 0) return TREE_RB;
    if (strcmp(s, "btree") == 0) return TREE_BTREE;
    return TREE_AVL; // !!! поставил временно AVL !!!
}

const char* typeName(TreeType type) {
    switch (type) {
        case TREE_RB:    return "rb";
        case TREE_BTREE: return "btree";
        default:         return "avl";
    }
}

// Парсинг одной строки JSONL


#define MAX_TOKENS_PER_DOC 4096
#define MAX_TOKEN_LEN      256

static int parse_jsonl_line(const char* line,
                             int*   doc_id,
                             char*  title,   size_t title_cap,
                             char** tokens,  int*   n_tokens) {
    // doc_id
    const char* p = strstr(line, "\"doc_id\"");
    if (!p) return 0;
    p = strchr(p, ':');
    if (!p) return 0;
    // doc_id может быть строкой "80" или числом 80
    while (*p == ':' || *p == ' ' || *p == '"') p++;
    *doc_id = atoi(p);

    // title
    p = strstr(line, "\"title\"");
    if (!p) return 0;
    p = strchr(p, ':');
    if (!p) return 0;
    p++;
    while (*p == ' ') p++;
    if (*p != '"') return 0;
    p++; // пропускаем открывающую кавычку

    size_t ti = 0;
    while (*p && *p != '"' && ti < title_cap - 1) {
        if (*p == '\\' && *(p + 1)) {
            p++; // пропускаем escape-символ
            // Базовые JSON escape-последовательности
            switch (*p) {
                case 'n':  title[ti++] = '\n'; break;
                case 't':  title[ti++] = '\t'; break;
                case '"':  title[ti++] = '"';  break;
                case '\\': title[ti++] = '\\'; break;
                default:   title[ti++] = *p;   break;
            }
        } else {
            title[ti++] = *p;
        }
        p++;
    }
    title[ti] = '\0';

    // tokens
    p = strstr(line, "\"tokens\"");
    if (!p) return 0;
    p = strchr(p, '[');
    if (!p) return 0;
    p++;

    *n_tokens = 0;
    while (*p && *p != ']') {
        while (*p == ' ' || *p == ',') p++;
        if (*p != '"') { p++; continue; }
        p++; // открывающая кавычка токена

        int ti2 = 0;
        while (*p && *p != '"' && ti2 < MAX_TOKEN_LEN - 1)
            tokens[*n_tokens][ti2++] = *p++;
        tokens[*n_tokens][ti2] = '\0';

        if (ti2 > 0) (*n_tokens)++;
        if (*p == '"') p++; // закрывающая кавычка
    }

    return 1;
}

// Индексация

static void runIndex(TreeType type, const char* data_path, const char* idx_path) {
    FILE* f = fopen(data_path, "r");
    if (!f) {
        fprintf(stderr, "Не удалось открыть %s\n", data_path);
        exit(EXIT_FAILURE);
    }

    Index* idx = createIndex(type);

    // Буферы для парсинга
    char  line[1024 * 64]; // строка JSONL до 64 КБ
    char  title[MAX_TITLE_LEN];
    char  token_storage[MAX_TOKENS_PER_DOC][MAX_TOKEN_LEN];
    char* token_ptrs[MAX_TOKENS_PER_DOC];
    for (int i = 0; i < MAX_TOKENS_PER_DOC; i++)
        token_ptrs[i] = token_storage[i];

    int doc_id, n_tokens;
    long docs = 0;

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '\n' || line[0] == '\0') continue;

        if (!parse_jsonl_line(line, &doc_id, title, sizeof(title),
                              token_ptrs, &n_tokens))
            continue;

        indexDocument(idx, doc_id, title,
                      (const char**)token_ptrs, n_tokens);
        docs++;

        if (docs % 10000 == 0)
            fprintf(stderr, "Indexed: %ld documents\n", docs);
    }
    fclose(f);

    clock_gettime(CLOCK_MONOTONIC, &t1);
    double elapsed = (t1.tv_sec  - t0.tv_sec)  * 1000.0
                   + (t1.tv_nsec - t0.tv_nsec) / 1e6;

    fprintf(stderr, "Done: %ld documents in %.1f ms\n", docs, elapsed);

    saveIndex(idx, idx_path);
    fprintf(stderr, "Index saved: %s\n", idx_path);

    freeIndex(idx);
}


static void runSearch(TreeType type, const char* idx_path,
                      const char* query, int json_out) {
    Index* idx = loadIndex(idx_path, type);
    if (!idx) {
        fprintf(stderr, "Не удалось загрузить индекс: %s\n", idx_path);
        exit(EXIT_FAILURE);
    }

    SearchResults* sr = search(idx, query);
    if (json_out) printResultsJSON(sr);
    else          printResultsText(sr);

    freeSearchResults(sr);
    freeIndex(idx);
}

static void usage(const char* prog) {
    fprintf(stderr,
        "Usage:\n"
        "  %s index  --type=<avl|rb|btree> [--data=PATH] [--index=PATH]\n"
        "  %s search --type=<avl|rb|btree> [--index=PATH] [--json] \"query\"\n",
        prog, prog);
}

int main(int argc, char* argv[]) {
    if (argc < 3) { usage(argv[0]); return 1; }

    const char* mode = argv[1];
    TreeType    type = TREE_AVL;
    const char* data_path = "data/processed/docs.jsonl";
    char        idx_path[512] = {0};
    int         json_out = 0;
    const char* query    = NULL;

    for (int i = 2; i < argc; i++) {
        if      (strncmp(argv[i], "--type=",  7) == 0) type = parseType(argv[i] + 7);
        else if (strncmp(argv[i], "--data=",  7) == 0) data_path = argv[i] + 7;
        else if (strncmp(argv[i], "--index=", 8) == 0)
            strncpy(idx_path, argv[i] + 8, sizeof(idx_path) - 1);
        else if (strcmp(argv[i], "--json")    == 0)    json_out = 1;
        else if (argv[i][0] != '-')                    query = argv[i];
    }

    if (idx_path[0] == '\0')
        snprintf(idx_path, sizeof(idx_path), "data/index_%s.txt", typeName(type));

    if (strcmp(mode, "index") == 0) {
        runIndex(type, data_path, idx_path);
    } else if (strcmp(mode, "search") == 0) {
        if (!query) { fprintf(stderr, "No query provided\n"); return 1; }
        runSearch(type, idx_path, query, json_out);
    } else {
        fprintf(stderr, "Unknown mode: %s\n", mode);
        usage(argv[0]);
        return 1;
    }
    return 0;
}
