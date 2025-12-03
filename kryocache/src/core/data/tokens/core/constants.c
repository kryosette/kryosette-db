/**
 * @file arena_constants.c
 * @brief Arena allocator constants implementation
 */

#include "/Users/dimaeremin/kryosette-db/kryocache/src/core/data/tokens/core/include/constants.h"
#include <unistd.h>

// ==================== Arena Configuration ====================
static const size_t ARENA_DEFAULT_CHUNK_SIZE = 64 * 1024;  // 64KB
static const size_t ARENA_MAX_CHUNK_SIZE = 2 * 1024 * 1024; // 2MB
static const size_t ARENA_ALIGNMENT = 8;

// ==================== Time Constants ====================
static const time_t ARENA_DEFAULT_MAX_AGE = 300; // 5 minutes

// ==================== Memory Constants ====================
static const size_t ARENA_MIN_ALLOC_SIZE = 1;
static const size_t ARENA_MAX_ALLOC_SIZE = ARENA_MAX_CHUNK_SIZE / 2;

// ==================== Initial Values ====================
static const size_t ARENA_INITIAL_CHUNK_COUNT = 0;
static const size_t ARENA_INITIAL_TOTAL_USED = 0;

// ==================== Arena Configuration Getters ====================
size_t get_arena_default_chunk_size(void) {
    return ARENA_DEFAULT_CHUNK_SIZE;
}

size_t get_arena_max_chunk_size(void) {
    return ARENA_MAX_CHUNK_SIZE;
}

size_t get_arena_alignment(void) {
    return ARENA_ALIGNMENT;
}

size_t get_arena_page_size(void) {
    static long page_size = 0;
    if (page_size == 0) {
        page_size = sysconf(_SC_PAGESIZE);
    }
    return (size_t)page_size;
}

// ==================== Time Constants Getters ====================
time_t get_arena_default_max_age(void) {
    return ARENA_DEFAULT_MAX_AGE;
}

// ==================== Memory Constants Getters ====================
size_t get_arena_min_alloc_size(void) {
    return ARENA_MIN_ALLOC_SIZE;
}

size_t get_arena_max_alloc_size(void) {
    return ARENA_MAX_ALLOC_SIZE;
}

// ==================== Initial Values Getters ====================
size_t get_arena_initial_chunk_count(void) {
    return ARENA_INITIAL_CHUNK_COUNT;
}

size_t get_arena_initial_total_used(void) {
    return ARENA_INITIAL_TOTAL_USED;
}