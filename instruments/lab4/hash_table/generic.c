#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "../../lab3/vector/generic.h"
#include "generic.h"

int HashInt(const void *key)
{
    if (!key) return 0;

    return *(int *)key;
}

int HashString(const void *key)
{
    if (!key) return 0;

    const char *str = key;
    unsigned long hash = 5381;
    int c;

    while ((c = *str++)) {
        hash = hash * 33 + c;
    }

    return (int)hash;
}

HashTable *createHashTable(size_t key_size, size_t val_size)
{
    HashTable *table = malloc(sizeof(HashTable));
    if (!table) return NULL;

    table->capacity = TABLE_MIN_SIZE;
    table->key_size = key_size;
    table->size = 0;
    table->val_size = val_size;

    size_t slot_size = sizeof(unsigned char) + key_size + val_size;

    table->values = createVector(slot_size);
    if (!table->values) return NULL;

    unsigned char *empty_slot = malloc(slot_size);
    memset(empty_slot, SLOT_EMPTY, slot_size);

    for (size_t i = 0; i < table->capacity; i++) {
        appendVectorItem(table->values, empty_slot);
    }

    free(empty_slot);

    return table;
}

void setItemHashTable(HashTable *table, void *key, void *data, HashFunc hash, CmpFunc cmp)
{
    if (!table || !key || !data || !hash || !cmp) return;

    if ((double)table->size / table->capacity > 0.5) {
        rehashHashTable(table, hash, cmp);
    }

    size_t h = (size_t)hash(key);
    size_t idx;
    long first_deleted = -1;

    for (size_t i = 0; i < table->capacity; i++) {
        idx = (h + i) % table->capacity;

        void *slot = getVectorItem(table->values, idx);
        unsigned char status = *(unsigned char *)slot;

        void *slot_key = (char *)slot + 1;
        void *slot_value = (char *)slot + 1 + table->key_size;

        if (status == SLOT_OCCUPIED && cmp(slot_key, key)) {
            memcpy(slot_value, data, table->val_size);
            return;
        }

        if (status == SLOT_DELETED && first_deleted == -1) {
            first_deleted = idx;
        }

        if (status == SLOT_EMPTY) {
            if (first_deleted != -1) {
                idx = first_deleted;
                slot = getVectorItem(table->values, idx);
            }

            *(unsigned char *)slot = SLOT_OCCUPIED;
            memcpy((char *)slot + 1, key, table->key_size);
            memcpy((char *)slot + 1 + table->key_size, data, table->val_size);

            table->size++;
            return;
        }
    }
}

void rehashHashTable(HashTable *table, HashFunc hash, CmpFunc cmp)
{
    size_t old_capacity = table->capacity;
    Vector *old_values = table->values;

    table->capacity = table->capacity * 2;
    table->size = 0;

    size_t slot_size = sizeof(unsigned char) + table->key_size + table->val_size;

    table->values = createVector(slot_size);

    if (!table->values) {
        table->values = old_values;
        return;
    }

    unsigned char *empty_slot = malloc(slot_size);
    memset(empty_slot, SLOT_EMPTY, slot_size);

    for (size_t i = 0; i < table->capacity; i++) {
        appendVectorItem(table->values, empty_slot);
    }
    
    free(empty_slot);

    for (size_t i = 0; i < old_capacity; i++) {
        void *slot = getVectorItem(old_values, i);
        unsigned char status = *(unsigned char *)slot;

        if (status == SLOT_OCCUPIED) {
            void *slot_key = (char *)slot + 1;
            void *slot_values = (char *)slot + 1 + table->key_size;
            setItemHashTable(table, slot_key, slot_values, hash, cmp);
        }
    }

    vectorFree(old_values);
}

void *getItemHashTable(HashTable *table, void *key, HashFunc hash, CmpFunc cmp)
{
    if (!table || !key || !hash || !cmp) return NULL;

    size_t h = (size_t)hash(key);
    size_t idx;

    for (size_t i = 0; i < table->capacity; i++) {
        idx = (h + i) % table->capacity;

        void *slot = getVectorItem(table->values, idx);
        unsigned char status = *(unsigned char *)slot;

        if (status == SLOT_OCCUPIED) {
            void *slot_key = (char *)slot + 1;
            void *slot_value = (char *)slot + 1 + table->key_size;

            if (cmp(slot_key, key)) {
                return slot_value;
            }
        }
    }

    return NULL;
}

void *popItemHashTable(HashTable *table, void *key, HashFunc hash, CmpFunc cmp)
{
    if (!table || !key || !hash || !cmp) return NULL;

    size_t h = (size_t)hash(key);
    size_t idx;

    for (size_t i = 0; i < table->capacity; i++) {
        idx = (h + i) % table->capacity;

        void *slot = getVectorItem(table->values, idx);
        unsigned char status = *(unsigned char *)slot;

        if (status == SLOT_EMPTY) return NULL;

        if (status == SLOT_OCCUPIED) {
            void *slot_key = (char *)slot + 1;
            void *slot_value = (char *)slot + 1 + table->key_size;

            if (cmp(slot_key, key)) {
                void *copy = malloc(table->val_size);
                if (!copy) return NULL;

                memcpy(copy, slot_value, table->val_size);

                *(unsigned char *)slot = SLOT_DELETED;
                table->size--;

                return copy;
            }
        }
    }

    return NULL;
}

unsigned long int getCollisionCount(HashTable *table, HashFunc hash)
{
    if (!table || !hash) return 0;

    unsigned long int collisions = 0;

    for (size_t i = 0; i < table->capacity; i++) {
        void *slot = getVectorItem(table->values, i);
        unsigned char status = *(unsigned char *)slot;

        if (status == SLOT_OCCUPIED) {
            void *slot_key = (char *)slot + 1;
            size_t ideal_idx = (size_t)hash(slot_key) % table->capacity;

            if (ideal_idx != i) {
                collisions++;
            }
        }
    }

    return collisions;
}

void freeHashTable(HashTable *table)
{
    if (!table) return;

    vectorFree(table->values);
    free(table);
}
