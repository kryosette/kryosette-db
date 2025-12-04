#pragma once

#include "core.h"
#include "constants.h"

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

typedef struct secure_segment {
    void *data;
    void *metadata;
    void *audit_log;
    size_t size;
    int fd_crypto;
} secure_segment_t;

typedef struct client_auth {
    uint8_t password_hash[SHA256_DIGEST_LENGTH];
    uint8_t otp_secret[32];
    uint8_t certificate[2048];
    uint64_t last_login;
    uint32_t failed_attempts;
} client_auth_t;

typedef enum {

} certificate;

/*
PERMISSIONS
*/
typedef struct 
{
   PERM_READ = 1 << 0;
   PERM_WRITE = 1 << 1;
   PERM_DELETE = 1 << 2;
   PERM_ADMIN = 1 << 3;
   PERM_AUDIT = 1 << 4;
} permissions_t;

typedef struct {
    char resource[64];
    uint64_t max_attempts;
    uint64_t req_permissions;
} access_control_entry;

typedef struct {

} encrypted_storage_t;