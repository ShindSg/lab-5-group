#include "generic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

GenericList *createList(size_t elem_size)
{
    GenericList *list = malloc(sizeof(GenericList));

    if (!list) {
        printf("Memory allocation failed (GenericList)");
        exit(EXIT_FAILURE);
    }

    list->head = NULL;
    list->elem_size = elem_size;

    return list;
}

void appendItem(GenericList *list, void *data)
{
    if (!list || !data) return; // O(1)

    Node *node = malloc(sizeof(Node)); // O(1)

    if (!node) { // O(1)
        printf("Memory allocation failed (Node)"); // O(1)
        exit(EXIT_FAILURE); // O(1)
    }

    node->data = malloc(list->elem_size); // O(1)

    if (!node->data) { // O(1)
        printf("Momory allocation failed (data)"); // O(1)
        exit(EXIT_FAILURE); // O(1)
    }

    memcpy(node->data, data, list->elem_size); // O(1)
    node->next = NULL; // O(1)

    if (!list->head) { // O(1)
        list->head = node; // O(1)
        return; // O(1)
    }

    Node *cur = list->head; // O(1)
    
    while (cur->next) { // O(N)
        cur = cur->next; // O(1)
    }

    cur->next = node; // O(1)

    /*
    Оценка сверху - O(N)
    Точная оценка - Θ(N / 2)
    Оценка снизу - Ω(1)
    */
}

int findItem(GenericList *list, void *value, EqualsFunc cmp)
{
    if (!list || !value || !cmp) return -1; // O(1)

    Node *cur = list->head; // O(1)
    int index = 0; // O(1)

    while (cur) { // O(N)
        if (cmp(cur->data, value)) return index; // O(N)
        cur = cur->next; // O(1)
        index++; // O(1)
    }

    return -1; // O(1)

    /*
    Оценка сверху - O(N)
    Точная оценка - Θ(N / 2)
    Оценка снизу - Ω(1)
    */
}

void *popItem(GenericList *list, size_t index)
{
    if (!list || !list->head) return NULL; // O(1)

    if (index == 0) { // O(1)
        Node *old = list->head; // O(1)
        list->head = old->next; // O(1)

        void *copy = malloc(list->elem_size); // O(1)

        if (!copy) { // O(1)
            printf("Memory allocation failed (pop copy)"); // O(1)
            exit(EXIT_FAILURE); // O(1)
        }

        memcpy(copy, old->data, list->elem_size); // O(1)

        free(old->data); // O(1)
        free(old); // O(1)

        return copy; // O(1)
    }

    Node *cur = list->head; // O(1)
    size_t i = 0; // O(1)

    while (cur->next && i < index - 1) { // O(N)
        cur = cur->next; // O(1)
        i++; // O(1)
    }

    if (!cur->next) return NULL; // O(1)

    Node *toDelete = cur->next; // O(1)
    cur->next = toDelete->next; // O(1)

    void *copy = malloc(list->elem_size); // O(1)

    if (!copy) { // O(1)
        printf("Memory allocation failed (pop copy)"); // O(1)
        exit(EXIT_FAILURE); // O(1)
    }

    memcpy(copy, toDelete->data, list->elem_size); // O(1)

    free(toDelete->data); // O(1)
    free(toDelete); // O(1)

    return copy; // O(1)

    /*
    Оценка сверху - O(N)
    Точная оценка - Θ(N / 2)
    Оценка снизу - Ω(1)
    */
}

void freeList(GenericList *list)
{
    if (!list) return;

    Node *cur = list->head;

    while (cur) {
        Node *next = cur->next;

        free(cur->data);
        free(cur);

        cur = next;
    }

    free(list);
}

unsigned int listLength(GenericList *list)
{
    if (!list) return 0; // O(1)

    unsigned count = 0; // O(1)
    Node *cur = list->head; // O(1)

    while (cur) { // O(N)
        cur = cur->next; // O(1)
        count++; // O(1)
    }

    return count; // O(1)

    /*
    Оценка сверху - O(N)
    Точная оценка - Θ(N / 2)
    Оценка снизу - Ω(1)
    */
}
