/**
 * @file core.h
 * @brief Core structures and definitions for arena allocator
 */
#pragma once

#include <pthread.h>
#include <stddef.h>
#include <time.h>
#include "constants.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char* id;
    char* user_id;
    char* username;
    char** authorities;
    size_t authorities_count;
    char* device_hash;
    time_t issued_at;
    time_t expires_at;
    char* client_ip;
} token_data_t;

typedef struct arena_chunk {
    void* memory;
    size_t used;
    size_t size;
    time_t created_at;
    struct arena_chunk* next;
} arena_chunk_t;

typedef struct arena_memory {
    arena_chunk_t* current;
    arena_chunk_t* old_chunks;
    pthread_mutex_t lock;
    size_t total_allocated;
    size_t total_used;
    size_t chunk_count;
    time_t last_cleanup;
} arena_memory_t;

arena_memory_t* arena_init(void);
void* arena_alloc(arena_memory_t* arena, size_t size);
void arena_reset(arena_memory_t* arena);
void arena_cleanup_old(arena_memory_t* arena, time_t max_age);
void arena_destroy(arena_memory_t* arena);

size_t arena_get_total_used(const arena_memory_t* arena);
size_t arena_get_total_allocated(const arena_memory_t* arena);
size_t arena_get_chunk_count(const arena_memory_t* arena);

#ifdef __cplusplus
}
#endif