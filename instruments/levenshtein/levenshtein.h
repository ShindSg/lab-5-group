#ifndef LEVENSHTEIN_H
#define LEVENSHTEIN_H

#include <stddef.h>


typedef enum {
    OP_NONE,      // Символы совпали, ничего не делаем
    OP_INSERT,    // Вставка символа
    OP_DELETE,    // Удаление символа
    OP_REPLACE    // Замена символа
} OperationType;

typedef struct {
    OperationType type;
    int position;     // Позиция в исходной строке
    char oldChar;     // Старый символ (для DELETE и REPLACE)
    char newChar;     // Новый символ (для INSERT и REPLACE)
} EditOperation;

typedef struct {
    int distance;              // Расстояние Левенштейна
    EditOperation *operations; // Массив операций
    int operationCount;        // Количество операций
} EditResult;

// Вычисление расстояния Левенштейна
// Возвращает минимальное количество операций для превращения s1 в s2
int levenshteinDistance(const char *s1, const char *s2);

// Вычисление расстояния + восстановление последовательности операций
// Возвращает NULL при ошибке
EditResult *levenshteinWithOperations(const char *s1, const char *s2);

// Печать таблицы DP
void printEditTable(int **dp, const char *s1, const char *s2);

// Печать последовательности операций
void printOperations(EditResult *result, const char *s1, const char *s2);

// Пошаговая демонстрация преобразования строки
void printTransformation(EditResult *result, const char *s1, const char *s2);

// Найти все слова из словаря с расстоянием <= maxDistance
// Возвращает массив строк (копии из словаря)
// resultCount — количество найденных слов
// Возвращает NULL при ошибке или если ничего не найдено
char **findSimilarWords(const char *word, char **dictionary, int dictSize,
                        int maxDistance, int *resultCount);

// Освобождение результата findSimilarWords
void freeSimilarWords(char **words, int count);

void freeEditResult(EditResult *result);

#endif // LEVENSHTEIN_H
