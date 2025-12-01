#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct storage_node_db
{
    char key[64];
    char value[256];
    struct storage_node_db *next;
} storage_node_db_t;

typedef struct {
    storage_node_db_t *buckets[1000];
    size_t size;
} storage_db_t;

static storage_db_t g_storage = {0};

// temp (improve!)
static unsigned int hash(const char *key) {
    unsigned int hash = 0;
    while (*key) {
        hash = (hash * 31) + *key++;
    }

    return hash & 1000;
}

void handle_client_connection(int client_fd);
bool storage_set(const char *key, const char *value);