#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "levenshtein.h"

static int minInt(int a, int b)
{
    return (a < b) ? a : b;
}

static int minOfThree(int a, int b, int c)
{
    return minInt(minInt(a, b), c);
}

static int **allocateTable(int rows, int cols)
{
    if (rows <= 0 || cols <= 0) {
        return NULL;
    }

    int **Table = malloc(sizeof(int*) * rows);
    if (!Table) {
        return NULL;
    }

    for (int i = 0; i < rows; i++) {
        Table[i] = calloc(cols, sizeof(int));
        if (!Table[i]) {
            for (int j = 0; j < i; j++) {
                free(Table[j]);
            }
            free(Table);

            return NULL;
        }

        if (i == 0) {
            for (int j = 0; j < cols; j++) {
                Table[i][j] = j;
            }
        } else {
            Table[i][0] = i;
        }
    }

    return Table;
}

static void freeTable(int **table, int rows)
{
    if (!table || rows <= 0) {
        return;
    }

    for (int i = 0; i < rows; i++) {
        free(table[i]);
    }

    free(table);
}

static int **buildLevenshteinTable(const char *s1, const char *s2)
{
    int rows = strlen(s1) + 1;
    int cols = strlen(s2) + 1;

    int **table = allocateTable(rows, cols);
    if (!table) {
        return NULL;
    }

    for (int i = 1; i < rows; i++) {
        for (int j = 1; j < cols; j++) {
            if (s1[i - 1] == s2[j - 1]) {
                table[i][j] = table[i - 1][j - 1];
            } else {
                int del = table[i - 1][j];
                int rep = table[i - 1][j - 1];
                int ins = table[i][j - 1];

                table[i][j] = minOfThree(del, ins, rep) + 1;
            }
        }
    }

    return table;
}

static void runDemo(void)
{
    const char *s1 = "kitten";
    const char *s2 = "sitting";

    int rows = strlen(s1) + 1;

    int **table = buildLevenshteinTable(s1, s2);

    EditResult *result = levenshteinWithOperations(s1, s2);
    if (!result) {
        printf("Failed to build edit result.\n");
        return;
    }

    printf("=== DEMO MODE ===\n\n");
    printf("String 1: \"%s\"\n", s1);
    printf("String 2: \"%s\"\n", s2);
    printf("Distance: %d\n\n", result->distance);

    printEditTable(table, s1, s2);
    printOperations(result, s1, s2);
    printTransformation(result, s1, s2);

    freeEditResult(result);
    freeTable(table, rows);
}

static void runDictionaryDemo(void)
{
    char *dictionary[] = {
        "algorithm",
        "algorithms",
        "altruism",
        "logarithm",
        "programming",
        "computer",
        "function",
        "distance",
        "editing",
        "algoritm"
    };

    const char *word = "algorithm";
    int dictSize = (int)(sizeof(dictionary) / sizeof(dictionary[0]));
    int maxDistance = 2;
    int resultCount = 0;

    char **similar = findSimilarWords(word, dictionary, dictSize, maxDistance, &resultCount);

    printf("=== DICTIONARY MODE ===\n\n");
    printf("Word: \"%s\"\n", word);
    printf("Max distance: %d\n\n", maxDistance);

    if (!similar) {
        printf("No similar words found.\n");
        return;
    }

    printf("Similar words (%d):\n", resultCount);
    for (int i = 0; i < resultCount; i++) {
        int dist = levenshteinDistance(word, similar[i]);
        printf("  %d. %s (distance = %d)\n", i + 1, similar[i], dist);
    }
    printf("\n");

    freeSimilarWords(similar, resultCount);
}

static void test_basic_distance(void)
{
    assert(levenshteinDistance("cat", "cat") == 0);
    assert(levenshteinDistance("cat", "cut") == 1);
    assert(levenshteinDistance("kitten", "sitting") == 3);
    assert(levenshteinDistance("book", "back") == 2);
}

static void test_boundary_distance(void)
{
    assert(levenshteinDistance("", "") == 0);
    assert(levenshteinDistance("", "abc") == 3);
    assert(levenshteinDistance("abc", "") == 3);
    assert(levenshteinDistance("a", "b") == 1);
}

static void test_invalid_distance(void)
{
    assert(levenshteinDistance(NULL, "abc") == -1);
    assert(levenshteinDistance("abc", NULL) == -1);
    assert(levenshteinDistance(NULL, NULL) == -1);
}

static void test_operations_basic(void)
{
    EditResult *result = levenshteinWithOperations("cat", "cut");

    assert(result != NULL);
    assert(result->distance == 1);
    assert(result->operationCount == 1);
    assert(result->operations != NULL);
    assert(result->operations[0].type == OP_REPLACE);

    freeEditResult(result);
}

static void test_operations_boundary(void)
{
    EditResult *result1 = levenshteinWithOperations("", "");
    assert(result1 != NULL);
    assert(result1->distance == 0);
    assert(result1->operationCount == 0);
    freeEditResult(result1);

    EditResult *result2 = levenshteinWithOperations("", "abc");
    assert(result2 != NULL);
    assert(result2->distance == 3);
    freeEditResult(result2);
}

static void test_operations_invalid(void)
{
    assert(levenshteinWithOperations(NULL, "abc") == NULL);
    assert(levenshteinWithOperations("abc", NULL) == NULL);
    assert(levenshteinWithOperations(NULL, NULL) == NULL);
}

static void test_find_similar_basic(void)
{
    char *dictionary[] = { "cat", "car", "dog", "cart" };
    int count = 0;

    char **result = findSimilarWords("cat", dictionary, 4, 1, &count);

    assert(result != NULL);
    assert(count >= 2);

    freeSimilarWords(result, count);
}

static void test_find_similar_boundary(void)
{
    char *dictionary[] = { "cat", "dog", "cut" };
    int count = 0;

    char **result = findSimilarWords("cat", dictionary, 3, 0, &count);

    assert(result != NULL);
    assert(count == 1);
    assert(strcmp(result[0], "cat") == 0);

    freeSimilarWords(result, count);
}

static void test_find_similar_invalid(void)
{
    int count = 123;
    char *dictionary[] = { "cat", "dog" };

    assert(findSimilarWords(NULL, dictionary, 2, 1, &count) == NULL);
    assert(findSimilarWords("cat", NULL, 2, 1, &count) == NULL);
    assert(findSimilarWords("cat", dictionary, 0, 1, &count) == NULL);
    assert(findSimilarWords("cat", dictionary, 2, -1, &count) == NULL);
    assert(findSimilarWords("cat", dictionary, 2, 1, NULL) == NULL);
}

static void runTests(void)
{
    printf("Running tests...\n");

    test_basic_distance();
    test_boundary_distance();
    test_invalid_distance();

    test_operations_basic();
    test_operations_boundary();
    test_operations_invalid();

    test_find_similar_basic();
    test_find_similar_boundary();
    test_find_similar_invalid();

    printf("All tests passed.\n");
}

int main(int argc, char *argv[])
{
    if (argc > 1) {
        if (strcmp(argv[1], "--demo") == 0) {
            runDemo();
            return 0;
        }

        if (strcmp(argv[1], "--dictionary") == 0) {
            runDictionaryDemo();
            return 0;
        }

        printf("Unknown argument: %s\n", argv[1]);
        printf("Usage:\n");
        printf("  ./tests            Run all tests\n");
        printf("  ./tests --demo     Show edit operations demo\n");
        printf("  ./tests --dictionary  Show dictionary search demo\n");
        return 1;
    }

    runTests();
    return 0;
}
