# Задача 2: Расстояние Левенштейна (Edit Distance)

## Описание

Расстояние Левенштейна — минимальное количество операций редактирования для превращения одной строки в другую. Допустимые операции:
- **Вставка** символа
- **Удаление** символа
- **Замена** символа

Используется повсеместно: автокоррекция, поиск похожих слов, diff в Git, сравнение ДНК.

## Рекуррентное соотношение

```
dp[i][j] = минимальное расстояние между s1[0..i-1] и s2[0..j-1]

Если s1[i-1] == s2[j-1]:
    dp[i][j] = dp[i-1][j-1]  // символы совпали, ничего не делаем
Иначе:
    dp[i][j] = 1 + min(
        dp[i-1][j],      // удаление из s1
        dp[i][j-1],      // вставка в s1
        dp[i-1][j-1]     // замена
    )

База:
    dp[i][0] = i  // удалить все символы из s1
    dp[0][j] = j  // вставить все символы из s2
```

## Файлы

```
levenshtein/
├── levenshtein.h    # Заголовочный файл
├── levenshtein.c    # Реализация
├── tests.c          # Тесты
└── README.md        # Этот файл
```

## Сборка и запуск

### Запуск тестов

```bash
gcc -Wall -Wextra -std=c11 -o tests levenshtein.c tests.c
./tests
```

### Демонстрация работы (желательно)

```bash
./tests --demo
```

Выводит:
1. Преобразование SATURDAY → SUNDAY с пошаговой визуализацией
2. Исправление опечаток с поиском похожих слов в словаре

## API

### Типы операций

```c
typedef enum {
    OP_NONE,      // Символы совпали
    OP_INSERT,    // Вставка
    OP_DELETE,    // Удаление
    OP_REPLACE    // Замена
} OperationType;
```

### Структуры

```c
typedef struct {
    OperationType type;
    int position;     // Позиция в исходной строке
    char oldChar;     // Старый символ
    char newChar;     // Новый символ
} EditOperation;

typedef struct {
    int distance;              // Расстояние
    EditOperation *operations; // Массив операций
    int operationCount;        // Количество операций
} EditResult;
```

### Основные функции

```c
// Только расстояние — O(m*n) времени, O(n) памяти
int levenshteinDistance(const char *s1, const char *s2);

// Расстояние + операции — O(m*n) времени, O(m*n) памяти
EditResult *levenshteinWithOperations(const char *s1, const char *s2);

// Поиск похожих слов в словаре
char **findSimilarWords(const char *word, char **dictionary, int dictSize,
                        int maxDistance, int *resultCount);
```

### Визуализация

```c
// Печать таблицы DP
void printEditTable(int **dp, const char *s1, const char *s2);

// Печать списка операций
void printOperations(EditResult *result, const char *s1, const char *s2);

// Пошаговая демонстрация преобразования
void printTransformation(EditResult *result, const char *s1, const char *s2);
```

## Пример использования

```c
#include "levenshtein.h"
#include <stdio.h>

int main()
{
    const char *s1 = "kitten";
    const char *s2 = "sitting";

    // Только расстояние
    int dist = levenshteinDistance(s1, s2);
    printf("Расстояние: %d\n", dist);  // 3

    // С операциями
    EditResult *result = levenshteinWithOperations(s1, s2);
    printOperations(result, s1, s2);
    printTransformation(result, s1, s2);
    freeEditResult(result);

    return 0;
}
```

## Пример вывода

```
=== SATURDAY -> SUNDAY ===

Расстояние Левенштейна: 3

Операции (3 шт.):
  1. DELETE 'A' из позиции 1
  2. DELETE 'T' из позиции 2
  3. REPLACE 'R' -> 'N' в позиции 4

Пошаговое преобразование:

  Начало:  "SATURDAY"
  Шаг 1:   "STURDAY"  (DELETE 'A' из 1)
  Шаг 2:   "SURDAY"  (DELETE 'T' из 2)
  Шаг 3:   "SUNDAY"  (REPLACE 'R'->'N' в 4)
  Итог:    "SUNDAY"
```

## Сложность

| Функция | Время | Память |
|---------|-------|--------|
| `levenshteinDistance` | O(m × n) | O(n) |
| `levenshteinWithOperations` | O(m × n) | O(m × n) |
| `findSimilarWords` | O(k × m × n) | O(n) |

где m, n — длины строк, k — размер словаря.

## Применение: исправление опечаток

```c
char *dictionary[] = {"algorithm", "programming", "computer", "function"};
int dict_size = 4;

int count = 0;
char **similar = findSimilarWords("algortihm", dictionary, dict_size, 2, &count);

// similar = ["algorithm"], count = 1

freeSimilarWords(similar, count);
```
