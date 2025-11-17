#pragma once

/**
 * @file server.h
 * @brief High-performance in-memory cache server implementation
 *
 * This module provides a Redis-like in-memory key-value store server
 * with support for multiple clients, persistence, and various data structures.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <netinet/in.h>

    /* ===== Data Types and Constants ===== */

    typedef struct sockaddr_in sockaddr_in_t;

    /**
     * @defgroup server_constants Server Constants
     * @{
     */

    /**
     * @brief Server operation modes
     */
    typedef enum
    {
        SERVER_MODE_STANDALONE, /**< Single server instance */
        SERVER_MODE_CLUSTER,    /**< Cluster node mode */
        SERVER_MODE_REPLICA     /**< Read-only replica mode */
    } server_mode_t;

    /**
     * @brief Server status codes
     */
    typedef enum
    {
        SERVER_STATUS_IDLE,          /**< Server is idle */
        SERVER_STATUS_STOPPED,       /**< Server is stopped */
        SERVER_STATUS_STARTING,      /**< Server is starting up */
        SERVER_STATUS_RUNNING,       /**< Server is running normally */
        SERVER_STATUS_SHUTTING_DOWN, /**< Server is shutting down */
        SERVER_STATUS_ERROR          /**< Server encountered an error */
    } server_status_t;

    /**
     * @brief Server configuration structure
     */
    typedef struct
    {
        uint32_t port;              /**< TCP port to listen on */
        uint32_t max_clients;       /**< Maximum client connections */
        size_t max_memory;          /**< Maximum memory usage in bytes */
        server_mode_t mode;         /**< Server operation mode */
        const char *bind_address;   /**< IP address to bind to */
        const char *data_directory; /**< Directory for persistence files */
        bool persistence_enabled;   /**< Enable data persistence to disk */
        int persistence_interval;   /**< Persistence interval in seconds */
    } server_config_t;

    /**
     * @brief Server statistics structure
     */
    typedef struct
    {
        uint64_t connections_total;  /**< Total client connections accepted */
        uint64_t commands_processed; /**< Total commands processed */
        uint64_t keys_stored;        /**< Current number of keys stored */
        size_t memory_used;          /**< Current memory usage in bytes */
        uint32_t connected_clients;  /**< Currently connected clients */
        double uptime_seconds;       /**< Server uptime in seconds */
    } server_stats_t;

    // =================== Foundation =====================

    typedef struct storage_node
    {
        char key[256];           // вместо MAX_SIZE_KEY
        char value[1024 * 1024]; // вместо MAX_SIZE_VALUE
        time_t expires_at;
        struct storage_node *next;
    } storage_node_t;

    typedef struct
    {
        storage_node_t **buckets;
        size_t capacity;
        size_t size;
        pthread_mutex_t lock;
    } storage_t;

    typedef struct client_context
    {
        int fd;
        sockaddr_in_t addr;
        pthread_t thread_id;
        bool connected;
    } client_context_t;

    typedef struct server_instance
    {
        server_config_t config;
        server_status_t status;
        storage_t *storage;
        int server_fd;
        pthread_t acceptor_thread;
        client_context_t *clients;
        uint32_t client_count;
        pthread_mutex_t clients_lock;
        char last_error[256];
        time_t start_time;
    } server_instance_t;

    /**
     * @brief Server instance handle
     */
    typedef struct server_instance server_instance_t;

    /** @} */

    /* ===== Core Server API ===== */

    /**
     * @defgroup server_core Core Server Management
     * @{
     */

    server_instance_t *server_init_default(void);
    server_instance_t *server_init(const server_config_t *config);
    bool server_start(server_instance_t *server);
    bool server_stop(server_instance_t *server, uint32_t timeout_ms);
    void server_force_shutdown(server_instance_t *server);
    void server_destroy(server_instance_t *server);

    /** @} */

    /* ===== Server Information API ===== */

    /**
     * @defgroup server_info Server Information
     * @{
     */

    server_status_t server_get_status(const server_instance_t *server);
    bool server_get_stats(const server_instance_t *server, server_stats_t *stats);
    const server_config_t *server_get_config(const server_instance_t *server);
    const char *server_get_last_error(const server_instance_t *server);

    /** @} */

    /* ===== Configuration API ===== */

    /**
     * @defgroup server_config Configuration Management
     * @{
     */

    server_config_t server_config_default(void);
    bool server_config_validate(const server_config_t *config,
                                char *error_buffer,
                                size_t error_size);
    bool server_config_load(const char *filename,
                            server_config_t *config,
                            char *error_buffer,
                            size_t error_size);
    bool server_config_save(const char *filename, const server_config_t *config);

    /** @} */

    /* ===== Advanced Features ===== */

    /**
     * @defgroup server_advanced Advanced Features
     * @{
     */

    bool server_save_data(server_instance_t *server);
    bool server_load_data(server_instance_t *server);
    bool server_flush_data(server_instance_t *server);
    const char *server_get_version(void);
    const char *server_get_build_info(void);

    /** @} */

#ifdef __cplusplus
}
#endif