#pragma once

#include "core.h"

typedef struct {
    const char *id; 
    const char *user_id; 
    const char *username; 
    const char **authorities; 
    size_t auth_count;
    const char *device_hash; 
    const char *client_ip; 
    time_t issued_at;
    time_t expires_at;
} token_metadata_t;

typedef struct {
    kryocache_context_t *cache;
    arena_memory_t *arena;  
    arena_memory_t *token_id_arena; 
    long token_expiration;
    char *issuer;
    pthread_mutex_t lock;
} token_t;