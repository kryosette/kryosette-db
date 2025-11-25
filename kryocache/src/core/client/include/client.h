/**
 * @file client.h
 * @brief High-performance in-memory cache client implementation
 *
 * This module provides a Redis-like client for connecting to kryocache server
 * with support for basic key-value operations and connection management.
 */

#pragma once

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

    /**
     * @defgroup client_constants Client Constants
     * @{
     */

    /**
     * @brief Client connection status codes
     */
    typedef enum
    {
        CLIENT_STATUS_DISCONNECTED, /**< Client is disconnected */
        CLIENT_STATUS_CONNECTING,   /**< Client is establishing connection */
        CLIENT_STATUS_CONNECTED,    /**< Client is connected and ready */
        CLIENT_STATUS_ERROR         /**< Client encountered an error */
    } client_status_t;

    /**
     * @brief Client operation results
     */
    typedef enum
    {
        CLIENT_SUCCESS,            /**< Operation completed successfully */
        CLIENT_ERROR_CONNECTION,   /**< Connection error */
        CLIENT_ERROR_TIMEOUT,      /**< Operation timeout */
        CLIENT_ERROR_PROTOCOL,     /**< Protocol error */
        CLIENT_ERROR_SERVER,       /**< Server returned error */
        CLIENT_ERROR_MEMORY,       /**< Memory allocation error */
        CLIENT_ERROR_INVALID_PARAM /**< Invalid parameter error */
    } client_result_t;

    /**
     * @brief Client configuration structure
     */
    typedef struct
    {
        const char *host;     /**< Server hostname or IP address */
        uint32_t port;        /**< Server port */
        uint32_t timeout_ms;  /**< Operation timeout in milliseconds */
        uint32_t max_retries; /**< Maximum connection retries */
        bool auto_reconnect;  /**< Enable automatic reconnection */
    } client_config_t;

    /**
     * @brief Client statistics structure
     */
    typedef struct
    {
        uint64_t operations_total;      /**< Total operations performed */
        uint64_t operations_failed;     /**< Total failed operations */
        uint64_t bytes_sent;            /**< Total bytes sent to server */
        uint64_t bytes_received;        /**< Total bytes received from server */
        uint32_t reconnect_count;       /**< Number of reconnections */
        double connection_time_seconds; /**< Total connection time */
    } client_stats_t;

    /**
     * @brief Client instance structure
     */
    typedef struct client_instance
    {
        client_config_t config;          /**< Client configuration */
        client_status_t status;          /**< Current connection status */
        int sockfd;                      /**< Socket file descriptor */
        struct sockaddr_in6 server_addr; /**< Server address */
        pthread_mutex_t lock;            /**< Client operation lock */
        client_stats_t stats;            /**< Client statistics */
        char last_error[256];            /**< Last error message */
        time_t connect_time;             /**< Connection establishment time */
        time_t last_activity;            /**< Last operation time */
    } client_instance_t;

    /** @} */

    /* ===== Core Client API ===== */

    /**
     * @defgroup client_core Core Client Management
     * @{
     */

    client_instance_t *client_init_default(void);
    client_instance_t *client_init(const client_config_t *config);
    client_result_t client_connect(client_instance_t *client);
    client_result_t client_disconnect(client_instance_t *client);
    void client_destroy(client_instance_t *client);

    /** @} */

    /* ===== Client Operations API ===== */

    /**
     * @defgroup client_ops Client Operations
     * @{
     */

    client_result_t client_set(client_instance_t *client,
                               const char *key,
                               const char *value);
    client_result_t client_get(client_instance_t *client,
                               const char *key,
                               char *value_buffer,
                               size_t buffer_size);
    client_result_t client_delete(client_instance_t *client, const char *key);
    client_result_t client_exists(client_instance_t *client, const char *key);
    client_result_t client_flush(client_instance_t *client);
    client_result_t client_ping(client_instance_t *client);

    /** @} */

    /* ===== Client Information API ===== */

    /**
     * @defgroup client_info Client Information
     * @{
     */

    client_status_t client_get_status(const client_instance_t *client);
    bool client_get_stats(const client_instance_t *client, client_stats_t *stats);
    const client_config_t *client_get_config(const client_instance_t *client);
    const char *client_get_last_error(const client_instance_t *client);
    bool client_is_connected(const client_instance_t *client);
    static bool check_connection_complete_poll(int sockfd, int timeout_ms);

    /** @} */

    /* ===== Utility Functions ===== */

    /**
     * @defgroup client_utils Utility Functions
     * @{
     */

    client_config_t client_config_default(void);
    bool client_config_validate(const client_config_t *config,
                                char *error_buffer,
                                size_t error_size);
    const char *client_result_to_string(client_result_t result);

    /** @} */

#ifdef __cplusplus
}
#endif