#include "levenshtein.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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

// Для отладки — возвращает имя операции
const char *operationName(OperationType type)
{
    switch (type)
    {
        case OP_NONE:    return "NONE";
        case OP_INSERT:  return "INSERT";
        case OP_DELETE:  return "DELETE";
        case OP_REPLACE: return "REPLACE";
        default:         return "UNKNOWN";
    }
}


int levenshteinDistance(const char *s1, const char *s2)
{
    if (!s1 || !s2) {
        return -1;
    }

    int rows = strlen(s1) + 1;
    int cols = strlen(s2) + 1;

    int *cur_row = malloc(sizeof(int) * cols);
    int *prev_row = malloc(sizeof(int) * cols);
    if (!prev_row || !cur_row) {
        free(cur_row);
        free(prev_row);
        return -1;
    }

    for (int j = 0; j < cols; j++) {
        prev_row[j] = j;
    }

    for (int i = 1; i < rows; i++) {
        cur_row[0] = i;

        for (int j = 1; j < cols; j++) {
            if (s1[i - 1] == s2[j - 1]) {
                cur_row[j] = prev_row[j - 1];
            } else {
                int to_delete = prev_row[j];
                int to_replace = prev_row[j - 1];
                int to_insert = cur_row[j - 1];

                cur_row[j] = minOfThree(to_delete, to_insert, to_replace) + 1;
            }
        }
        int *tmp = prev_row;
        prev_row = cur_row;
        cur_row = tmp;
    }

    int result = prev_row[cols - 1];

    free(cur_row);
    free(prev_row);

    return result;
}

EditResult *levenshteinWithOperations(const char *s1, const char *s2)
{
    if (!s1 || !s2) {
        return NULL;
    }

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
                int to_delete = table[i - 1][j];
                int to_replace = table[i - 1][j - 1];
                int to_insert = table[i][j - 1];

                table[i][j] = minOfThree(to_delete, to_insert, to_replace) + 1;
            }
        }
    }

    int distance = table[rows - 1][cols - 1];

    EditResult *result = malloc(sizeof(EditResult));
    if (!result) {
        freeTable(table, rows);
        return NULL;
    }

    result->distance = distance;
    result->operationCount = 0;
    result->operations = NULL;

    if (distance == 0) {
        freeTable(table, rows);
        return result;
    }

    EditOperation *ops = malloc(distance * sizeof(EditOperation));
    if (!ops) {
        free(result);
        freeTable(table, rows);
        return NULL;
    }

    int i = rows - 1;
    int j = cols - 1;
    int count = 0;

    while (i > 0 || j > 0) {
        if (i > 0 && j > 0 &&
            s1[i - 1] == s2[j - 1] &&
            table[i][j] == table[i - 1][j - 1]) {
            i--;
            j--;
        }
        else if (i > 0 && j > 0 &&
                 table[i][j] == table[i - 1][j - 1] + 1) {
            ops[count].type = OP_REPLACE;
            ops[count].position = i - 1;
            ops[count].oldChar = s1[i - 1];
            ops[count].newChar = s2[j - 1];
            count++;
            i--;
            j--;
        }
        else if (i > 0 && table[i][j] == table[i - 1][j] + 1) {
            ops[count].type = OP_DELETE;
            ops[count].position = i - 1;
            ops[count].oldChar = s1[i - 1];
            ops[count].newChar = '\0';
            count++;
            i--;
        }
        else if (j > 0 && table[i][j] == table[i][j - 1] + 1) {
            ops[count].type = OP_INSERT;
            ops[count].position = i;
            ops[count].oldChar = '\0';
            ops[count].newChar = s2[j - 1];
            count++;
            j--;
        }
    }

    for (int l = 0, r = count - 1; l < r; l++, r--) {
        EditOperation tmp = ops[l];
        ops[l] = ops[r];
        ops[r] = tmp;
    }

    result->operations = ops;
    result->operationCount = count;

    freeTable(table, rows);

    return result;
}


void printEditTable(int **dp, const char *s1, const char *s2)
{
    if (!dp || !s1 || !s2) {
        return;
    }

    int rows = strlen(s1) + 1;
    int cols = strlen(s2) + 1;

    printf("DP table:\n\n");

    printf("     \"\" ");
    for (int j = 0; j < cols - 1; j++) {
        printf("%3c ", s2[j]);
    }
    printf("\n");

    printf(" \"\" ");
    for (int j = 0; j < cols; j++) {
        printf("%3d ", dp[0][j]);
    }
    printf("\n");

    for (int i = 1; i < rows; i++) {
        printf("%3c ", s1[i - 1]);

        for (int j = 0; j < cols; j++) {
            printf("%3d ", dp[i][j]);
        }

        printf("\n");
    }

    printf("\n");
}

void printOperations(EditResult *result, const char *s1, const char *s2)
{
    if (!result || !s1 || !s2) {
        return;
    }

    printf("Operations for \"%s\" -> \"%s\":\n\n", s1, s2);

    for (int i = 0; i < result->operationCount; i++) {
        EditOperation op = result->operations[i];

        printf("%d. %s ", i + 1, operationName(op.type));

        if (op.type == OP_INSERT) {
            printf("'%c' in position %d", op.newChar, op.position);
        }
        else if (op.type == OP_DELETE) {
            printf("'%c' from position %d", op.oldChar, op.position);
        }
        else if (op.type == OP_REPLACE) {
            printf("'%c' -> '%c' in position %d",
                   op.oldChar, op.newChar, op.position);
        }

        printf("\n");
    }

    printf("\n");
}

void printTransformation(EditResult *result, const char *s1, const char *s2)
{
    if (!result || !s1 || !s2) {
        return;
    }

    printf("Transformation \"%s\" -> \"%s\":\n\n", s1, s2);

    char buffer[256];
    strcpy(buffer, s1);

    printf("Start: \"%s\"\n", buffer);

    for (int i = 0; i < result->operationCount; i++) {
        EditOperation op = result->operations[i];

        if (op.type == OP_DELETE) {
            memmove(&buffer[op.position],
                    &buffer[op.position + 1],
                    strlen(buffer) - op.position);
        }
        else if (op.type == OP_INSERT) {
            int len = strlen(buffer);

            memmove(&buffer[op.position + 1],
                    &buffer[op.position],
                    len - op.position + 1);

            buffer[op.position] = op.newChar;
        }
        else if (op.type == OP_REPLACE) {
            buffer[op.position] = op.newChar;
        }

        printf("Step %d: \"%s\" (%s",
               i + 1,
               buffer,
               operationName(op.type));

        if (op.type == OP_REPLACE) {
            printf(" '%c'->'%c' in %d", op.oldChar, op.newChar, op.position);
        }
        else if (op.type == OP_DELETE) {
            printf(" '%c' from %d", op.oldChar, op.position);
        }
        else if (op.type == OP_INSERT) {
            printf(" '%c' in %d", op.newChar, op.position);
        }

        printf(")\n");
    }

    printf("Result: \"%s\"\n\n", buffer);
}

char **findSimilarWords(const char *word, char **dictionary, int dictSize,
                        int maxDistance, int *resultCount)
{
    if (!word || !dictionary || dictSize <= 0 || maxDistance < 0 || !resultCount) {
        return NULL;
    }

    char **result = malloc(sizeof(char*) * dictSize);
    if (!result) {
        return NULL;
    }

    int count = 0;
    for (int i = 0; i < dictSize; i++) {
        if (!dictionary[i]) {
            continue;
        }

        int distance = levenshteinDistance(word, dictionary[i]);
        if (0 <= distance && distance <= maxDistance) {
            result[count] = strdup(dictionary[i]);
            if (!result[count]) {
                for (int j = 0; j < count; j++) {
                    free(result[j]);
                }
                free(result);

                return NULL;
            }

            count++;
        }
    }

    if (count == 0) {
        free(result);
        *resultCount = 0;
        return NULL;
    }

    *resultCount = count;
    return result;
}

void freeSimilarWords(char **words, int count)
{
    if (!words) {
        return;
    }

    for (int i = 0; i < count; i++) {
        free(words[i]);
    }

    free(words);
}


void freeEditResult(EditResult *result)
{
    if (!result) {
        return;
    }

    free(result->operations);
    free(result);
}
