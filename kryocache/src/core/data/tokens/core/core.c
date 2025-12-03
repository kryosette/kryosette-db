/**
 * @file arena.c
 * @brief Arena allocator implementation
 */

#include "/Users/dimaeremin/kryosette-db/kryocache/src/core/data/tokens/core/include/core.h"
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <errno.h>

static arena_chunk_t* arena_chunk_create(size_t size);
static void arena_chunk_destroy(arena_chunk_t* chunk);
static size_t arena_align_size(size_t size);
static size_t arena_calculate_new_chunk_size(size_t requested_size);

/**
 * @brief Initialize a new arena allocator
 * @return Pointer to initialized arena, or NULL on failure
 */
arena_memory_t* arena_init(void) {
    arena_memory_t* arena = (arena_memory_t*)calloc(1, sizeof(arena_memory_t));
    if (!arena) {
        return NULL;
    }

    if (pthread_mutex_init(&arena->lock, NULL) != 0) {
        free(arena);
        return NULL;
    }

    arena->current = NULL;
    arena->old_chunks = NULL;
    arena->total_allocated = 0;
    arena->total_used = get_arena_initial_total_used();
    arena->chunk_count = get_arena_initial_chunk_count();
    arena->last_cleanup = time(NULL);

    return arena;
}

/**
 * @brief Create a new memory chunk
 * @param size Requested size of the chunk
 * @return Pointer to created chunk, or NULL on failure
 */
static arena_chunk_t* arena_chunk_create(size_t size) {
    if (size == 0) {
        return NULL;
    }

    // Allocate chunk structure
    arena_chunk_t* chunk = (arena_chunk_t*)calloc(1, sizeof(arena_chunk_t));
    if (!chunk) {
        return NULL;
    }

    // Align size to page boundary
    size_t page_size = get_arena_page_size();
    size_t aligned_size = ((size + page_size - 1) / page_size) * page_size;

    // Allocate memory using mmap
    chunk->memory = mmap(NULL, aligned_size,
                        PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS,
                        -1, 0);
    
    if (chunk->memory == MAP_FAILED) {
        free(chunk);
        return NULL;
    }

    chunk->size = aligned_size;
    chunk->used = 0;
    chunk->created_at = time(NULL);
    chunk->next = NULL;

    return chunk;
}

/**
 * @brief Destroy a memory chunk
 * @param chunk Pointer to chunk to destroy
 */
static void arena_chunk_destroy(arena_chunk_t* chunk) {
    if (!chunk) {
        return;
    }

    if (chunk->memory && chunk->size > 0) {
        memset(chunk->memory, 0, chunk->used);
        munmap(chunk->memory, chunk->size);
    }

    free(chunk);
}

/**
 * @brief Align size to arena alignment boundary
 * @param size Original size
 * @return Aligned size
 */
static size_t arena_align_size(size_t size) {
    size_t alignment = get_arena_alignment();
    return ((size + alignment - 1) / alignment) * alignment;
}

/**
 * @brief Calculate appropriate size for new chunk
 * @param requested_size Size requested by user
 * @return Calculated chunk size
 */
static size_t arena_calculate_new_chunk_size(size_t requested_size) {
    size_t default_size = get_arena_default_chunk_size();
    size_t max_size = get_arena_max_chunk_size();

    if (requested_size <= default_size) {
        return default_size;
    }

    // Round up to next power of two, but cap at max size
    size_t size = default_size;
    while (size < requested_size && size < max_size) {
        if (size > max_size / 2) {
            return max_size;
        }
        size <<= 1;
    }

    return (size > max_size) ? max_size : size;
}

/**
 * @brief Allocate memory from arena
 * @param arena Arena allocator instance
 * @param size Size to allocate
 * @return Pointer to allocated memory, or NULL on failure
 */
void* arena_alloc(arena_memory_t* arena, size_t size) {
    if (!arena || size == 0) {
        return NULL;
    }

    if (size > get_arena_max_alloc_size()) {
        return NULL;
    }

    pthread_mutex_lock(&arena->lock);

    size = arena_align_size(size);

    // Check if current chunk has enough space
    if (!arena->current || (arena->current->used + size > arena->current->size)) {
        // Try to find a suitable chunk in old chunks list
        arena_chunk_t** chunk_ptr = &arena->old_chunks;
        arena_chunk_t* suitable_chunk = NULL;
        arena_chunk_t* prev_chunk = NULL;

        while (*chunk_ptr) {
            arena_chunk_t* current_chunk = *chunk_ptr;
            if (current_chunk->size - current_chunk->used >= size) {
                suitable_chunk = current_chunk;
                // Remove from old chunks list
                if (prev_chunk) {
                    prev_chunk->next = current_chunk->next;
                } else {
                    arena->old_chunks = current_chunk->next;
                }
                break;
            }
            prev_chunk = current_chunk;
            chunk_ptr = &current_chunk->next;
        }

        if (suitable_chunk) {
            // Move current to old chunks if it has used memory
            if (arena->current && arena->current->used > 0) {
                arena->current->next = arena->old_chunks;
                arena->old_chunks = arena->current;
            } else if (arena->current) {
                // Destroy empty current chunk
                arena_chunk_destroy(arena->current);
                arena->chunk_count--;
            }
            
            arena->current = suitable_chunk;
        } else {
            // Create new chunk
            size_t new_chunk_size = arena_calculate_new_chunk_size(size);
            arena_chunk_t* new_chunk = arena_chunk_create(new_chunk_size);
            
            if (!new_chunk) {
                pthread_mutex_unlock(&arena->lock);
                return NULL;
            }

            // Move current to old chunks if it has used memory
            if (arena->current && arena->current->used > 0) {
                arena->current->next = arena->old_chunks;
                arena->old_chunks = arena->current;
            } else if (arena->current) {
                // Destroy empty current chunk
                arena_chunk_destroy(arena->current);
                arena->chunk_count--;
            }

            arena->current = new_chunk;
            arena->total_allocated += new_chunk->size;
            arena->chunk_count++;
        }
    }

    // Allocate from current chunk
    void* ptr = (char*)arena->current->memory + arena->current->used;
    arena->current->used += size;
    arena->total_used += size;

    pthread_mutex_unlock(&arena->lock);
    return ptr;
}

/**
 * @brief Reset arena, freeing all allocated memory
 * @param arena Arena allocator instance
 */
void arena_reset(arena_memory_t* arena) {
    if (!arena) {
        return;
    }

    pthread_mutex_lock(&arena->lock);

    // Destroy all old chunks
    arena_chunk_t* chunk = arena->old_chunks;
    while (chunk) {
        arena_chunk_t* next = chunk->next;
        arena_chunk_destroy(chunk);
        arena->chunk_count--;
        chunk = next;
    }
    arena->old_chunks = NULL;

    // Reset current chunk if it exists
    if (arena->current) {
        arena->current->used = 0;
    }

    arena->total_used = 0;
    arena->last_cleanup = time(NULL);

    pthread_mutex_unlock(&arena->lock);
}

/**
 * @brief Clean up old chunks based on age
 * @param arena Arena allocator instance
 * @param max_age Maximum age in seconds for chunks to keep
 */
void arena_cleanup_old(arena_memory_t* arena, time_t max_age) {
    if (!arena || max_age <= 0) {
        return;
    }

    pthread_mutex_lock(&arena->lock);

    time_t now = time(NULL);
    arena_chunk_t** chunk_ptr = &arena->old_chunks;

    while (*chunk_ptr) {
        arena_chunk_t* current_chunk = *chunk_ptr;
        
        if (now - current_chunk->created_at > max_age) {
            // Remove and destroy old chunk
            *chunk_ptr = current_chunk->next;
            arena_chunk_destroy(current_chunk);
            arena->chunk_count--;
        } else {
            // Move to next chunk
            chunk_ptr = &current_chunk->next;
        }
    }

    arena->last_cleanup = now;

    pthread_mutex_unlock(&arena->lock);
}

/**
 * @brief Destroy arena and all associated resources
 * @param arena Arena allocator instance
 */
void arena_destroy(arena_memory_t* arena) {
    if (!arena) {
        return;
    }

    pthread_mutex_lock(&arena->lock);

    // Destroy all chunks
    arena_chunk_t* chunk = arena->old_chunks;
    while (chunk) {
        arena_chunk_t* next = chunk->next;
        arena_chunk_destroy(chunk);
        chunk = next;
    }

    if (arena->current) {
        arena_chunk_destroy(arena->current);
    }

    pthread_mutex_unlock(&arena->lock);
    pthread_mutex_destroy(&arena->lock);
    free(arena);
}

/**
 * @brief Get total memory used in arena
 * @param arena Arena allocator instance
 * @return Total bytes used
 */
size_t arena_get_total_used(const arena_memory_t* arena) {
    return arena ? arena->total_used : 0;
}

/**
 * @brief Get total memory allocated by arena
 * @param arena Arena allocator instance
 * @return Total bytes allocated
 */
size_t arena_get_total_allocated(const arena_memory_t* arena) {
    return arena ? arena->total_allocated : 0;
}

/**
 * @brief Get number of chunks in arena
 * @param arena Arena allocator instance
 * @return Number of chunks
 */
size_t arena_get_chunk_count(const arena_memory_t* arena) {
    return arena ? arena->chunk_count : 0;
}