/**
 * @file constants.h
 * @brief Server constants definition header
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

/**
 * @file constants.h
 * @brief Comprehensive server constants definition header
 *
 * This header provides a complete set of constants for the in-memory cache server
 * including configuration defaults, error codes, and protocol specifications.
 */

#ifdef __cplusplus
extern "C"
{
#endif

    // ==================== Server Default Values ====================
    uint16_t get_server_default_port(void);    ///< Default server port (Redis-compatible)
    uint32_t get_server_max_clients(void);     ///< Maximum simultaneous client connections
    size_t get_server_buffer_size(void);       ///< Default I/O buffer size in bytes
    size_t get_max_key_length(void);           ///< Maximum key length in bytes
    size_t get_max_value_length(void);         ///< Maximum value length (1MB)
    size_t get_initial_storage_capacity(void); ///< Initial storage capacity
    size_t get_initial_storage_size(void);     ///< Initial storage size

    // ==================== Server Status Constants ====================
    server_status_t get_initial_server_status(void); ///< Initial server status
    int get_server_status_stopped(void);             ///< Server stopped status code
    int get_server_status_running(void);             ///< Server running status code
    int get_server_status_shutting_down(void);       ///< Server shutting down status code

    // ==================== Threading Constants ====================
    int get_mutex_success_code(void);      ///< Mutex operation success code
    int get_initial_server_fd(void);       ///< Initial server file descriptor
    pthread_t get_initial_thread_id(void); ///< Initial thread ID

    // ==================== Client Constants ====================
    uint32_t get_max_clients_count(const server_config_t *config); ///< Max clients from config
    uint32_t get_initial_client_count(void);                       ///< Initial client count

    // ==================== Utility Constants ====================
    const char *get_initial_error_message(void); ///< Initial error message
    time_t get_initial_start_time(void);         ///< Initial start time

    // ==================== Server Error Codes ====================
    int get_server_err_success(void);         ///< Operation completed successfully
    int get_server_err_unknown_command(void); ///< Command not recognized by server
    int get_server_err_invalid_args(void);    ///< Invalid arguments provided to command
    int get_server_err_memory_full(void);     ///< Server memory limit reached
    int get_server_err_key_not_found(void);   ///< Requested key does not exist
    int get_server_err_key_exists(void);      ///< Key already exists
    int get_server_err_unauthorized(void);    ///< Client not authorized for operation
    int get_server_err_system(void);          ///< Internal system error

    // ==================== Protocol Constants ====================
    uint8_t get_protocol_version(void);           ///< Current protocol version
    uint8_t get_protocol_magic_byte(void);        ///< Magic byte for protocol identification
    uint8_t get_protocol_handshake_timeout(void); ///< Handshake timeout in seconds

    // ==================== Performance Constants ====================
    size_t get_initial_hash_table_size(void);    ///< Initial size of hash table buckets
    size_t get_max_hash_table_size(void);        ///< Maximum size of hash table
    uint32_t get_cleanup_interval_seconds(void); ///< Background cleanup interval
    uint32_t get_client_timeout_seconds(void);   ///< Client connection timeout

    // ==================== Command Constants ====================
    size_t get_max_command_length(void);           ///< Maximum length of a single command
    size_t get_max_args_per_command(void);         ///< Maximum arguments per command
    uint32_t get_max_command_execution_time(void); ///< Maximum command execution time in ms

#ifdef __cplusplus
}
#endif