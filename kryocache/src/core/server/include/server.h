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

// Include constants
#include "constants.h"

    /* ===== Data Types and Constants ===== */

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
        uint16_t port;              /**< TCP port to listen on */
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
        char key[MAX_SIZE_KEY];
        char value[MAX_SIZE_VALUE];
        time_t expires_at;
        struct storage_node *next;
    } storage_node_t;

    /*
buckets is an array of pointers
storage_node_t** buckets = [ptr0, ptr1, ptr2, ..., ptr1023];
                      ↑    ↑    ↑           ↑
                      │    │    │           └─── buckets[1023] → NULL
                      │    │    └─── buckets[2] → node1 → node2 → NULL
                      │    └─── buckets[1] → node1 → NULL
                      └─── buckets[0] → node1 → node2 → node3 → NULL
    */
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
        struct sockaddr_in addr;
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

    /* ===== Constant Accessor Functions ===== */

    /**
     * @defgroup server_constants_accessors Server Constant Accessors
     * @{
     */

    // Server default values
    uint16_t get_server_default_port(void);
    uint32_t get_server_max_clients(void);
    size_t get_server_buffer_size(void);
    size_t get_max_key_length(void);
    size_t get_max_value_length(void);

    // Server error codes
    int get_server_err_success(void);
    int get_server_err_unknown_command(void);
    int get_server_err_invalid_args(void);
    int get_server_err_memory_full(void);
    int get_server_err_key_not_found(void);
    int get_server_err_key_exists(void);
    int get_server_err_unauthorized(void);
    int get_server_err_system(void);

    // Protocol constants
    uint8_t get_protocol_version(void);
    uint8_t get_protocol_magic_byte(void);
    uint8_t get_protocol_handshake_timeout(void);

    /** @} */

    /* ===== Core Server API ===== */

    /**
     * @defgroup server_core Core Server Management
     * @{
     */

    /**
     * @brief Initialize server with default configuration
     *
     * @return New server instance or NULL on failure
     */
    server_instance_t *server_init_default(void);

    /**
     * @brief Initialize server with custom configuration
     *
     * @param config Server configuration parameters
     * @return New server instance or NULL on failure
     */
    server_instance_t *server_init(const server_config_t *config);

    /**
     * @brief Start the server
     *
     * Begins accepting client connections and processing commands.
     *
     * @param server Server instance
     * @return true if server started successfully, false otherwise
     */
    bool server_start(server_instance_t *server);

    /**
     * @brief Gracefully stop the server
     *
     * Stops accepting new connections and waits for existing clients to disconnect.
     *
     * @param server Server instance
     * @param timeout_ms Maximum time to wait for graceful shutdown
     * @return true if shutdown successful, false if timeout or error
     */
    bool server_stop(server_instance_t *server, uint32_t timeout_ms);

    /**
     * @brief Force immediate server shutdown
     *
     * Immediately terminates all connections and stops the server.
     *
     * @param server Server instance
     */
    void server_force_shutdown(server_instance_t *server);

    /**
     * @brief Destroy server instance and free all resources
     *
     * @param server Server instance to destroy
     */
    void server_destroy(server_instance_t *server);

    /** @} */

    /* ===== Server Information API ===== */

    /**
     * @defgroup server_info Server Information
     * @{
     */

    /**
     * @brief Get current server status
     *
     * @param server Server instance
     * @return Current server status
     */
    server_status_t server_get_status(const server_instance_t *server);

    /**
     * @brief Get server statistics
     *
     * @param server Server instance
     * @param stats Output parameter for statistics
     * @return true if statistics retrieved successfully
     */
    bool server_get_stats(const server_instance_t *server, server_stats_t *stats);

    /**
     * @brief Get server configuration
     *
     * @param server Server instance
     * @return Pointer to server configuration (read-only)
     */
    const server_config_t *server_get_config(const server_instance_t *server);

    /**
     * @brief Get last error message
     *
     * @param server Server instance
     * @return Error message string (NULL if no error)
     */
    const char *server_get_last_error(const server_instance_t *server);

    /** @} */

    /* ===== Configuration API ===== */

    /**
     * @defgroup server_config Configuration Management
     * @{
     */

    /**
     * @brief Create default server configuration
     *
     * @return Default configuration structure
     */
    server_config_t server_config_default(void);

    /**
     * @brief Validate server configuration
     *
     * @param config Configuration to validate
     * @param error_buffer Buffer for error message
     * @param error_size Size of error buffer
     * @return true if configuration is valid, false otherwise
     */
    bool server_config_validate(const server_config_t *config,
                                char *error_buffer,
                                size_t error_size);

    /**
     * @brief Load configuration from file
     *
     * @param filename Configuration file path
     * @param config Output configuration structure
     * @param error_buffer Buffer for error message
     * @param error_size Size of error buffer
     * @return true if configuration loaded successfully
     */
    bool server_config_load(const char *filename,
                            server_config_t *config,
                            char *error_buffer,
                            size_t error_size);

    /**
     * @brief Save configuration to file
     *
     * @param filename Configuration file path
     * @param config Configuration to save
     * @return true if configuration saved successfully
     */
    bool server_config_save(const char *filename, const server_config_t *config);

    /** @} */

    /* ===== Event Handlers and Callbacks ===== */

    /**
     * @defgroup server_callbacks Event Callbacks
     * @{
     */

    /**
     * @brief Client connection event callback
     *
     * @param client_id Unique client identifier
     * @param client_ip Client IP address
     * @param user_data User-provided context data
     */
    typedef void (*server_on_connect_cb)(int client_id, const char *client_ip, void *user_data);

    /**
     * @brief Client disconnection event callback
     *
     * @param client_id Unique client identifier
     * @param user_data User-provided context data
     */
    typedef void (*server_on_disconnect_cb)(int client_id, void *user_data);

    /**
     * @brief Command execution event callback
     *
     * @param client_id Unique client identifier
     * @param command Command string
     * @param user_data User-provided context data
     */
    typedef void (*server_on_command_cb)(int client_id, const char *command, void *user_data);

    /** @} */

    /* ===== Advanced Features ===== */

    /**
     * @defgroup server_advanced Advanced Features
     * @{
     */

    /**
     * @brief Perform immediate data persistence
     *
     * Forces immediate snapshot of in-memory data to disk.
     *
     * @param server Server instance
     * @return true if persistence successful
     */
    bool server_save_data(server_instance_t *server);

    /**
     * @brief Load data from persistence storage
     *
     * @param server Server instance
     * @return true if data loaded successfully
     */
    bool server_load_data(server_instance_t *server);

    /**
     * @brief Flush all data from server
     *
     * Removes all keys and values from memory.
     *
     * @param server Server instance
     * @return true if flush successful
     */
    bool server_flush_data(server_instance_t *server);

    /**
     * @brief Get server version information
     *
     * @return Version string
     */
    const char *server_get_version(void);

    /**
     * @brief Get server build information
     *
     * @return Build information string
     */
    const char *server_get_build_info(void);

    /** @} */

#ifdef __cplusplus
}
#endif