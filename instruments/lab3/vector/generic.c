#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "generic.h"

// Вспомогательная функция для изменения размера
static bool needToResize(Vector *vector, bool *increase)
{
    if (!vector || !increase) return false;

    if (vector->size == vector->capacity) {
        *increase = true;
        return true;
    }

    if (vector->size > 0 && vector->size <= vector->capacity / 4) {
        *increase = false;
        return true;
    }

    return false;
}

// Определяем увеличивать размер или уменьшать
static int resize(Vector *vector, bool increase)
{
    if (!vector) return -1;

    size_t new_capacity;

    if (increase) {
        new_capacity = vector->capacity * 2;
    } else {
        new_capacity = vector->capacity / 2;
        if (new_capacity < MIN_SIZE) {
            new_capacity = MIN_SIZE;
        }
    }

    void *new_bloc = realloc(vector->data, new_capacity * vector->elem_size);

    if (!new_bloc) return -1;

    vector->data = new_bloc;
    vector->capacity = new_capacity;

    return 0;
}

Vector *createVector(size_t elem_size)
{
    Vector *vector = malloc(sizeof(Vector));

    if (!vector) return NULL;

    vector->elem_size = elem_size;
    vector->capacity = MIN_SIZE;
    vector->size = 0;

    vector->data = malloc(vector->capacity * elem_size);

    if (!vector->data) {
        free(vector);
        return NULL;
    }
    
    return vector;
}

int appendVectorItem(Vector *vector, void *el)
{
    if (!vector || !el) return -1;  // O(1)

    bool increase = false;  // O(1)

    if (needToResize(vector, &increase)) {  // O(1)
        if (resize(vector, increase) != 0) return -1;  // O(N) из-за realloc в худшем случае
    }

    void *dest = (char*)vector->data + vector->size * vector->elem_size;  // O(1)
    memcpy(dest, el, vector->elem_size);  // O(1)

    vector->size++;  // O(1)

    return 0;  // O(1)
    /*
    Оценка сверху - O(N)
    Точная оценка  - Θ(N / 2) 
    Оценка снизу - Ω(1) 
    */
}

void *getVectorItem(Vector *vector, size_t index)
{
    if (!vector) return NULL;  // O(1)

    if (vector->size <= index) return NULL;  // O(1)

    return (char*)vector->data + index * vector->elem_size;  // O(1)
    /*
    Оценка сверху - O(1)
    Точная оценка - Θ(1)
    Оценка снизу - Ω(1)
    */
}

int setVectorItem(Vector *vector, size_t index, void *value)
{
    if (!vector || !value) return -1;  // O(1)

    if (vector->size <= index) return -1;  // O(1)

    void *dest = (char*)vector->data + index * vector->elem_size;  // O(1)
    memcpy(dest, value, vector->elem_size);  // O(1)

    return 0;  // O(1)
    /*
    Оценка сверху - O(1)
    Точная оценка - Θ(1)
    Оценка снизу - Ω(1)
    */
}

void *popVectorItem(Vector *vector, size_t index)
{
    if (!vector) return NULL;  // O(1)

    if (vector->size <= index) return NULL;  // O(1)

    void *copy = malloc(vector->elem_size);  // O(1)

    if (!copy) return NULL;  // O(1)

    void *toDelete = (char*)vector->data + index * vector->elem_size;  // O(1)
    memcpy(copy, toDelete, vector->elem_size);  // O(1)

    if (index < vector->size - 1) {  // O(1)
        void *first_after_toDelete = (char *)vector->data + (index + 1) * vector->elem_size;  // O(1)
        memmove(toDelete, first_after_toDelete,
                (vector->size - index - 1) * vector->elem_size);  // O(N)
    }

    vector->size--;  // O(1)

    bool increase = false;  // O(1)

    if (needToResize(vector, &increase)) {  // O(1)
        if (resize(vector, increase) != 0) {  // O(N) 
        }
    }

    return copy;  // O(1)
    /*
    Оценка сверху - O(N)
    Точная оценка  - Θ(N / 2) 
    Оценка снизу - Ω(1) 
    */
}

long int findVectorItem(Vector *vector, void *value, EqualsFunc cmp)
{
    if (!vector || !value) return -1;  // O(1)

    for (size_t i = 0; i < vector->size; i++) {  // O(N)
        void *el = (char*)vector->data + i * vector->elem_size;  // O(1)
        if (cmp(el, value)) return i;  // O(1)
    }

    return -1;  // O(1)
    /*
    Оценка сверху - O(N)
    Точная оценка  - Θ(N / 2)
    Оценка снизу - Ω(1) 
    */
}

int vectorFree(Vector *vector)
{
    if (!vector) return -1;

    free(vector->data);
    free(vector);

    return 0;
}
