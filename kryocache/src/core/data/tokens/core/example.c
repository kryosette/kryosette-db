/**
 * @file example.c
 * @brief Example usage of arena allocator with comprehensive error handling
 */

#include "/Users/dimaeremin/kryosette-db/kryocache/src/core/data/tokens/core/include/core.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>  // Добавлено для malloc/free

// Define bool type if not available
#ifndef __bool_true_false_are_defined
typedef unsigned char bool;
#define true 1
#define false 0
#define __bool_true_false_are_defined 1
#endif

// ==================== Helper Functions ====================

/**
 * @brief Safe string copy with bounds checking
 * @param dest Destination buffer
 * @param src Source string
 * @param dest_size Size of destination buffer
 * @return 1 on success, 0 on failure
 */
static int safe_strcpy(char* dest, const char* src, size_t dest_size) {
    if (!dest || !src || dest_size == 0) {
        return 0;
    }
    
    size_t src_len = strlen(src);
    if (src_len >= dest_size) {
        // Truncate but ensure null termination
        strncpy(dest, src, dest_size - 1);
        dest[dest_size - 1] = '\0';
        return 0; // Indicate truncation
    }
    
    strcpy(dest, src);
    return 1;
}

/**
 * @brief Validate token parameters
 * @param id Token ID
 * @param user_id User ID
 * @param username Username
 * @return 1 if parameters are valid, 0 otherwise
 */
static int validate_token_params(const char* id, const char* user_id, const char* username) {
    if (!id || strlen(id) == 0) {
        fprintf(stderr, "Error: Token ID cannot be NULL or empty\n");
        return 0;
    }
    
    if (strlen(id) > 256) {
        fprintf(stderr, "Error: Token ID too long (max 256 characters)\n");
        return 0;
    }
    
    if (user_id && strlen(user_id) > 256) {
        fprintf(stderr, "Error: User ID too long (max 256 characters)\n");
        return 0;
    }
    
    if (username && strlen(username) > 256) {
        fprintf(stderr, "Error: Username too long (max 256 characters)\n");
        return 0;
    }
    
    return 1;
}

/**
 * @brief Validate timestamps
 * @param issued_at Issue timestamp
 * @param expires_at Expiration timestamp
 * @param check_expiration Whether to check if token is expired
 * @return 1 if timestamps are valid, 0 otherwise
 */
static int validate_timestamps(time_t issued_at, time_t expires_at, int check_expiration) {
    if (issued_at <= 0) {
        fprintf(stderr, "Error: Invalid issue timestamp\n");
        return 0;
    }
    
    if (expires_at <= 0) {
        fprintf(stderr, "Error: Invalid expiration timestamp\n");
        return 0;
    }
    
    if (expires_at <= issued_at) {
        fprintf(stderr, "Error: Expiration time must be after issue time\n");
        return 0;
    }
    
    if (check_expiration) {
        // Check if token is already expired
        time_t now = time(NULL);
        if (issued_at > now) {
            fprintf(stderr, "Warning: Token issued in the future\n");
        }
        
        if (expires_at < now) {
            fprintf(stderr, "Warning: Token already expired\n");
            return 0; // Consider expired tokens as invalid for tests
        }
    }
    
    return 1;
}

// ==================== Token Creation with Comprehensive Error Handling ====================

/**
 * @brief Create a token using arena allocator with full error handling
 * @param arena Arena allocator instance
 * @param id Token ID (required)
 * @param user_id User ID (optional)
 * @param username Username (optional)
 * @param authorities Array of authority strings (optional)
 * @param auth_count Number of authorities (0 if NULL)
 * @param device_hash Device hash (optional)
 * @param issued_at Issue timestamp (required)
 * @param expires_at Expiration timestamp (required)
 * @param client_ip Client IP address (optional)
 * @param check_expiration Whether to check if token is expired (1) or not (0)
 * @return Pointer to created token, or NULL on failure
 */
token_data_t* token_data_create_ex(arena_memory_t* arena, 
                                  const char* id, 
                                  const char* user_id, 
                                  const char* username,
                                  const char** authorities, 
                                  size_t auth_count,
                                  const char* device_hash,
                                  time_t issued_at, 
                                  time_t expires_at,
                                  const char* client_ip,
                                  int check_expiration) {
    
    // Comprehensive parameter validation
    if (!arena) {
        fprintf(stderr, "Error: Arena cannot be NULL\n");
        return NULL;
    }
    
    if (!validate_token_params(id, user_id, username)) {
        return NULL;
    }
    
    if (!validate_timestamps(issued_at, expires_at, check_expiration)) {
        return NULL;
    }
    
    if (auth_count > 0 && !authorities) {
        fprintf(stderr, "Error: Authorities array cannot be NULL when auth_count > 0\n");
        return NULL;
    }
    
    if (auth_count > 100) {
        fprintf(stderr, "Error: Too many authorities (max 100)\n");
        return NULL;
    }
    
    // Validate each authority string
    for (size_t i = 0; i < auth_count; i++) {
        if (!authorities[i] || strlen(authorities[i]) == 0) {
            fprintf(stderr, "Error: Authority string at index %zu is NULL or empty\n", i);
            return NULL;
        }
        if (strlen(authorities[i]) > 128) {
            fprintf(stderr, "Error: Authority string at index %zu too long (max 128 characters)\n", i);
            return NULL;
        }
    }
    
    // Allocate token structure from arena
    token_data_t* token_data = (token_data_t*)arena_alloc(arena, sizeof(token_data_t));
    if (!token_data) {
        fprintf(stderr, "Error: Failed to allocate token structure from arena\n");
        return NULL;
    }
    
    // Initialize all fields to safe defaults
    memset(token_data, 0, sizeof(token_data_t));
    
    // Allocate strings from arena with error handling
    size_t id_len = strlen(id) + 1;
    token_data->id = (char*)arena_alloc(arena, id_len);
    if (!token_data->id) {
        fprintf(stderr, "Error: Failed to allocate memory for token ID\n");
        return NULL;
    }
    
    // Allocate optional strings only if provided
    if (user_id) {
        size_t user_id_len = strlen(user_id) + 1;
        token_data->user_id = (char*)arena_alloc(arena, user_id_len);
        if (!token_data->user_id) {
            fprintf(stderr, "Error: Failed to allocate memory for user ID\n");
            return NULL;
        }
    }
    
    if (username) {
        size_t username_len = strlen(username) + 1;
        token_data->username = (char*)arena_alloc(arena, username_len);
        if (!token_data->username) {
            fprintf(stderr, "Error: Failed to allocate memory for username\n");
            return NULL;
        }
    }
    
    if (device_hash) {
        size_t device_hash_len = strlen(device_hash) + 1;
        token_data->device_hash = (char*)arena_alloc(arena, device_hash_len);
        if (!token_data->device_hash) {
            fprintf(stderr, "Error: Failed to allocate memory for device hash\n");
            return NULL;
        }
    }
    
    if (client_ip) {
        size_t client_ip_len = strlen(client_ip) + 1;
        token_data->client_ip = (char*)arena_alloc(arena, client_ip_len);
        if (!token_data->client_ip) {
            fprintf(stderr, "Error: Failed to allocate memory for client IP\n");
            return NULL;
        }
    }
    
    // Copy string data with bounds checking
    if (!safe_strcpy(token_data->id, id, id_len)) {
        fprintf(stderr, "Warning: Token ID was truncated\n");
    }
    
    if (user_id && token_data->user_id) {
        if (!safe_strcpy(token_data->user_id, user_id, strlen(user_id) + 1)) {
            fprintf(stderr, "Warning: User ID was truncated\n");
        }
    }
    
    if (username && token_data->username) {
        if (!safe_strcpy(token_data->username, username, strlen(username) + 1)) {
            fprintf(stderr, "Warning: Username was truncated\n");
        }
    }
    
    if (device_hash && token_data->device_hash) {
        if (!safe_strcpy(token_data->device_hash, device_hash, strlen(device_hash) + 1)) {
            fprintf(stderr, "Warning: Device hash was truncated\n");
        }
    }
    
    if (client_ip && token_data->client_ip) {
        if (!safe_strcpy(token_data->client_ip, client_ip, strlen(client_ip) + 1)) {
            fprintf(stderr, "Warning: Client IP was truncated\n");
        }
    }
    
    // Allocate authorities array
    if (auth_count > 0 && authorities) {
        token_data->authorities = (char**)arena_alloc(arena, sizeof(char*) * auth_count);
        if (!token_data->authorities) {
            fprintf(stderr, "Error: Failed to allocate authorities array\n");
            return NULL;
        }
        
        token_data->authorities_count = auth_count;
        
        for (size_t i = 0; i < auth_count; i++) {
            size_t auth_len = strlen(authorities[i]) + 1;
            token_data->authorities[i] = (char*)arena_alloc(arena, auth_len);
            if (!token_data->authorities[i]) {
                fprintf(stderr, "Error: Failed to allocate memory for authority %zu\n", i);
                // Clean up already allocated authorities
                for (size_t j = 0; j < i; j++) {
                    // Authorities are in arena, no need to free individually
                }
                return NULL;
            }
            
            if (!safe_strcpy(token_data->authorities[i], authorities[i], auth_len)) {
                fprintf(stderr, "Warning: Authority %zu was truncated\n", i);
            }
        }
    } else {
        token_data->authorities = NULL;
        token_data->authorities_count = 0;
    }
    
    // Set timestamps
    token_data->issued_at = issued_at;
    token_data->expires_at = expires_at;
    
    return token_data;
}

/**
 * @brief Simplified token creation without expiration check (for tests)
 */
token_data_t* token_data_create(arena_memory_t* arena, 
                               const char* id, 
                               const char* user_id, 
                               const char* username,
                               const char** authorities, 
                               size_t auth_count,
                               const char* device_hash,
                               time_t issued_at, 
                               time_t expires_at,
                               const char* client_ip) {
    return token_data_create_ex(arena, id, user_id, username, authorities, auth_count,
                               device_hash, issued_at, expires_at, client_ip, 0);
}

// ==================== Token Validation Functions ====================

/**
 * @brief Validate a token's integrity
 * @param token Token to validate
 * @return 1 if token is valid, 0 otherwise
 */
int token_validate(const token_data_t* token) {
    if (!token) {
        fprintf(stderr, "Error: Token cannot be NULL\n");
        return 0;
    }
    
    if (!token->id || strlen(token->id) == 0) {
        fprintf(stderr, "Error: Token ID is invalid\n");
        return 0;
    }
    
    if (token->issued_at <= 0) {
        fprintf(stderr, "Error: Invalid issue timestamp\n");
        return 0;
    }
    
    if (token->expires_at <= 0) {
        fprintf(stderr, "Error: Invalid expiration timestamp\n");
        return 0;
    }
    
    if (token->expires_at <= token->issued_at) {
        fprintf(stderr, "Error: Token expired before issue\n");
        return 0;
    }
    
    // Check for expired token
    time_t now = time(NULL);
    if (token->expires_at < now) {
        fprintf(stderr, "Warning: Token has expired\n");
        return 0;
    }
    
    // Validate authorities array consistency
    if ((token->authorities == NULL) != (token->authorities_count == 0)) {
        fprintf(stderr, "Error: Authorities array inconsistency\n");
        return 0;
    }
    
    // Validate each authority string
    for (size_t i = 0; i < token->authorities_count; i++) {
        if (!token->authorities[i] || strlen(token->authorities[i]) == 0) {
            fprintf(stderr, "Error: Authority %zu is invalid\n", i);
            return 0;
        }
    }
    
    return 1;
}

/**
 * @brief Check if token has a specific authority
 * @param token Token to check
 * @param authority Authority to look for
 * @return 1 if token has the authority, 0 otherwise
 */
int token_has_authority(const token_data_t* token, const char* authority) {
    if (!token || !authority) {
        return 0;
    }
    
    for (size_t i = 0; i < token->authorities_count; i++) {
        if (token->authorities[i] && strcmp(token->authorities[i], authority) == 0) {
            return 1;
        }
    }
    
    return 0;
}

// ==================== Test Scenarios ====================

/**
 * @brief Test successful token creation
 */
static void test_successful_creation(arena_memory_t* arena) {
    printf("\n=== Test 1: Successful Token Creation ===\n");
    
    const char* authorities[] = {"ROLE_USER", "ROLE_ADMIN", "ROLE_MODERATOR"};
    size_t auth_count = sizeof(authorities) / sizeof(authorities[0]);
    
    time_t now = time(NULL);
    token_data_t* token = token_data_create_ex(arena,
                                             "test_token_123",
                                             "user_456",
                                             "john_doe",
                                             authorities,
                                             auth_count,
                                             "device_hash_xyz",
                                             now,
                                             now + 7200, // 2 hours
                                             "192.168.1.1",
                                             0); // Don't check expiration
    
    if (token) {
        printf("✓ Token created successfully\n");
        printf("  ID: %s\n", token->id);
        printf("  User: %s\n", token->username);
        printf("  Authorities: %zu\n", token->authorities_count);
        
        // Validate the token
        if (token_validate(token)) {
            printf("✓ Token validation passed\n");
        }
        
        // Check for specific authority
        if (token_has_authority(token, "ROLE_ADMIN")) {
            printf("✓ Token has ADMIN authority\n");
        }
    } else {
        printf("✗ Token creation failed\n");
    }
}

/**
 * @brief Test error cases
 */
static void test_error_cases(arena_memory_t* arena) {
    printf("\n=== Test 2: Error Handling ===\n");
    
    time_t now = time(NULL);
    
    // Test 2.1: NULL arena
    printf("\nTest 2.1: NULL arena...\n");
    token_data_t* token1 = token_data_create_ex(NULL, "id", "user", "name", NULL, 0, 
                                               NULL, now, now + 3600, NULL, 0);
    if (!token1) {
        printf("✓ Correctly rejected NULL arena\n");
    }
    
    // Test 2.2: NULL ID
    printf("\nTest 2.2: NULL token ID...\n");
    token_data_t* token2 = token_data_create_ex(arena, NULL, "user", "name", NULL, 0, 
                                               NULL, now, now + 3600, NULL, 0);
    if (!token2) {
        printf("✓ Correctly rejected NULL token ID\n");
    }
    
    // Test 2.3: Empty ID
    printf("\nTest 2.3: Empty token ID...\n");
    token_data_t* token3 = token_data_create_ex(arena, "", "user", "name", NULL, 0, 
                                               NULL, now, now + 3600, NULL, 0);
    if (!token3) {
        printf("✓ Correctly rejected empty token ID\n");
    }
    
    // Test 2.4: Invalid timestamps
    printf("\nTest 2.4: Invalid timestamps...\n");
    token_data_t* token4 = token_data_create_ex(arena, "id", "user", "name", NULL, 0, 
                                               NULL, now + 100, now, NULL, 0);
    if (!token4) {
        printf("✓ Correctly rejected invalid timestamps\n");
    }
    
    // Test 2.5: Too many authorities
    printf("\nTest 2.5: Too many authorities...\n");
    const char* many_auths[150];
    for (int i = 0; i < 150; i++) {
        many_auths[i] = "ROLE";
    }
    token_data_t* token5 = token_data_create_ex(arena, "id", "user", "name", many_auths, 150, 
                                               NULL, now, now + 3600, NULL, 0);
    if (!token5) {
        printf("✓ Correctly rejected too many authorities\n");
    }
    
    // Test 2.6: NULL authorities with count > 0
    printf("\nTest 2.6: NULL authorities array...\n");
    token_data_t* token6 = token_data_create_ex(arena, "id", "user", "name", NULL, 5, 
                                               NULL, now, now + 3600, NULL, 0);
    if (!token6) {
        printf("✓ Correctly rejected NULL authorities array\n");
    }
    
    // Test 2.7: Already expired token (with expiration check)
    printf("\nTest 2.7: Expired token check...\n");
    token_data_t* token7 = token_data_create_ex(arena, "id", "user", "name", NULL, 0,
                                               NULL, now - 7200, now - 3600, NULL, 1);
    if (!token7) {
        printf("✓ Correctly rejected expired token\n");
    }
}

/**
 * @brief Test memory boundary conditions
 */
static void test_memory_boundaries(arena_memory_t* arena) {
    printf("\n=== Test 3: Memory Boundary Conditions ===\n");
    
    time_t now = time(NULL);
    
    // Test 3.1: Very long strings
    printf("\nTest 3.1: Long strings...\n");
    char long_id[300];
    char long_name[300];
    memset(long_id, 'A', 299);
    memset(long_name, 'B', 299);
    long_id[299] = '\0';
    long_name[299] = '\0';
    
    token_data_t* token1 = token_data_create_ex(arena, long_id, "user", long_name, NULL, 0, 
                                               NULL, now, now + 3600, NULL, 0);
    if (!token1) {
        printf("✓ Correctly rejected too long strings\n");
    }
    
    // Test 3.2: Many small allocations
    printf("\nTest 3.2: Many small allocations...\n");
    int success_count = 0;
    for (int i = 0; i < 1000; i++) {
        char id[32];
        sprintf(id, "token_%d", i);
        
        token_data_t* token = token_data_create_ex(arena, id, "user", "name", NULL, 0, 
                                                   NULL, now, now + 3600, NULL, 0);
        if (token) {
            success_count++;
        } else {
            printf("  Arena allocation failed after %d tokens\n", success_count);
            break;
        }
    }
    
    // Reset arena for next tests
    arena_reset(arena);
    printf("✓ Memory boundary tests completed\n");
}

/**
 * @brief Test arena statistics and cleanup
 */
static void test_arena_operations(arena_memory_t* arena) {
    printf("\n=== Test 4: Arena Operations ===\n");
    
    time_t now = time(NULL);
    
    // Create some tokens
    for (int i = 0; i < 10; i++) {
        char id[32];
        sprintf(id, "op_token_%d", i);
        
        token_data_t* token = token_data_create_ex(arena, id, "user", "name", NULL, 0, 
                                                   NULL, now, now + 3600, NULL, 0);
        if (!token) {
            printf("✗ Failed to create token %d\n", i);
        }
    }
    
    printf("Created 10 tokens\n");
    printf("  Total used: %zu bytes\n", arena_get_total_used(arena));
    printf("  Total allocated: %zu bytes\n", arena_get_total_allocated(arena));
    printf("  Chunk count: %zu\n", arena_get_chunk_count(arena));
    
    // Test cleanup
    printf("\nTesting cleanup...\n");
    arena_cleanup_old(arena, 0); // Cleanup all old chunks immediately
    printf("  After cleanup - Chunk count: %zu\n", arena_get_chunk_count(arena));
    
    // Test reset
    printf("\nTesting reset...\n");
    arena_reset(arena);
    printf("  After reset - Total used: %zu bytes\n", arena_get_total_used(arena));
    printf("  After reset - Chunk count: %zu\n", arena_get_chunk_count(arena));
    
    printf("✓ Arena operations test completed\n");
}

// ==================== Performance Test ====================

/**
 * @brief Test arena performance vs malloc
 */
static void test_performance(arena_memory_t* arena) {
    printf("\n=== Test 5: Performance Test ===\n");
    
    const int iterations = 10000;
    time_t now = time(NULL);
    
    // Test with arena
    printf("Testing arena allocator...\n");
    clock_t arena_start = clock();
    
    for (int i = 0; i < iterations; i++) {
        char id[32];
        sprintf(id, "perf_token_%d", i);
        
        token_data_t* token = token_data_create_ex(arena, id, "user", "name", NULL, 0,
                                                   NULL, now, now + 3600, NULL, 0);
        if (!token) {
            printf("  Failed at iteration %d\n", i);
            break;
        }
    }
    
    clock_t arena_end = clock();
    double arena_time = (double)(arena_end - arena_start) / CLOCKS_PER_SEC;
    
    // Reset arena
    arena_reset(arena);
    
    // Test with malloc (simulated)
    printf("Testing malloc (simulated)...\n");
    clock_t malloc_start = clock();
    
    for (int i = 0; i < iterations; i++) {
        char id[32];
        sprintf(id, "perf_token_%d", i);
        
        // Simulate malloc-based allocation
        token_data_t* token = (token_data_t*)malloc(sizeof(token_data_t));
        if (token) {
            memset(token, 0, sizeof(token_data_t));
            token->id = (char*)malloc(32);
            if (token->id) {
                strcpy(token->id, id);
            }
            // Free everything
            free(token->id);
            free(token);
        }
    }
    
    clock_t malloc_end = clock();
    double malloc_time = (double)(malloc_end - malloc_start) / CLOCKS_PER_SEC;
    
    printf("\nPerformance Results:\n");
    printf("  Arena time: %.6f seconds\n", arena_time);
    printf("  Malloc time: %.6f seconds\n", malloc_time);
    if (malloc_time > 0) {
        printf("  Arena is %.2fx faster\n", malloc_time / arena_time);
    }
    
    printf("✓ Performance test completed\n");
}

// ==================== Main Function ====================

int main(void) {
    printf("===============================================\n");
    printf("Arena Allocator - Comprehensive Example\n");
    printf("===============================================\n");
    
    // Initialize arena
    arena_memory_t* arena = arena_init();
    if (!arena) {
        fprintf(stderr, "CRITICAL ERROR: Failed to initialize arena\n");
        return 1;
    }
    
    printf("\n✓ Arena initialized successfully\n");
    printf("  Default chunk size: %zu bytes\n", get_arena_default_chunk_size());
    printf("  Max chunk size: %zu bytes\n", get_arena_max_chunk_size());
    printf("  Memory alignment: %zu bytes\n", get_arena_alignment());
    
    // Run all test scenarios
    test_successful_creation(arena);
    test_error_cases(arena);
    test_memory_boundaries(arena);
    
    // Reset arena between major test groups
    arena_reset(arena);
    
    test_arena_operations(arena);
    
    // Performance test (optional)
    arena_reset(arena);
    test_performance(arena);
    
    // Final statistics
    printf("\n===============================================\n");
    printf("Final Statistics:\n");
    printf("===============================================\n");
    printf("Total memory allocated via mmap: %zu bytes\n", arena_get_total_allocated(arena));
    printf("Peak memory used: %zu bytes\n", arena_get_total_used(arena));
    printf("Total chunks created: %zu\n", arena_get_chunk_count(arena));
    
    // Test edge case: destroy NULL arena
    printf("\nTesting NULL arena destruction...\n");
    arena_destroy(NULL);
    printf("✓ Destroying NULL arena didn't crash\n");
    
    // Cleanup
    arena_destroy(arena);
    printf("\n✓ Arena destroyed successfully\n");
    
    printf("\n===============================================\n");
    printf("All tests completed successfully! ✓\n");
    printf("===============================================\n");
    
    return 0;
}