/**
 * @file arena_constants.h
 * @brief Arena allocator constants definition
 */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// ==================== Arena Configuration ====================
size_t get_arena_default_chunk_size(void);
size_t get_arena_max_chunk_size(void);
size_t get_arena_alignment(void);
size_t get_arena_page_size(void);

// ==================== Time Constants ====================
time_t get_arena_default_max_age(void);

// ==================== Memory Constants ====================
size_t get_arena_min_alloc_size(void);
size_t get_arena_max_alloc_size(void);

// ==================== Initial Values ====================
size_t get_arena_initial_chunk_count(void);
size_t get_arena_initial_total_used(void);

#ifdef __cplusplus
}
#endif