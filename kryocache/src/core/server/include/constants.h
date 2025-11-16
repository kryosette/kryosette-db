/**
 * @file constants.h
 * @brief Server constants definition header
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include "server.h"

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

    // ==================== Network Constants ====================
    int get_socket_protocol(void);         ///< Socket protocol
    int get_socket_success_code(void);     ///< Socket operation success code
    int get_socket_reuseaddr_option(void); ///< Socket reuseaddr option value
    int get_socket_level(void);            ///< Socket level for setsockopt
    int get_socket_domain(void);           ///< Socket domain (AF_INET)
    int get_socket_bind_address(void);     ///< Socket bind address (INADDR_ANY)
    int get_socket_backlog(void);          ///< Socket listen backlog
    int get_socket_shutdown_mode(void);    ///< Socket shutdown mode

    // ==================== Test Constants ====================
    int get_test_custom_port(void);            ///< Test custom port
    uint32_t get_test_max_clients(void);       ///< Test max clients
    size_t get_test_max_memory(void);          ///< Test max memory
    server_mode_t get_test_server_mode(void);  ///< Test server mode
    const char *get_test_bind_address(void);   ///< Test bind address
    const char *get_test_data_directory(void); ///< Test data directory
    bool get_test_persistence_enabled(void);   ///< Test persistence enabled
    int get_test_persistence_interval(void);   ///< Test persistence interval

    int get_invalid_port_number(void);       ///< Invalid port number for testing
    uint32_t get_invalid_client_count(void); ///< Invalid client count for testing
    int get_minimum_port_number(void);       ///< Minimum valid port number
    int get_maximum_port_number(void);       ///< Maximum valid port number
    uint32_t get_minimum_client_count(void); ///< Minimum valid client count

    int get_initial_test_count(void);        ///< Initial test count
    int get_initial_test_index(void);        ///< Initial test index
    int get_initial_connection_count(void);  ///< Initial connection count
    int get_initial_command_count(void);     ///< Initial command count
    size_t get_initial_memory_usage(void);   ///< Initial memory usage
    int get_string_start_index(void);        ///< String start index
    char get_string_terminator(void);        ///< String terminator character
    size_t get_error_buffer_size(void);      ///< Error buffer size
    int get_minimum_version_length(void);    ///< Minimum version string length
    int get_minimum_build_info_length(void); ///< Minimum build info string length

    int get_init_test_count(void);     ///< Initialization test count
    int get_config_test_count(void);   ///< Configuration test count
    int get_info_test_count(void);     ///< Information test count
    int get_advanced_test_count(void); ///< Advanced features test count

    int get_polling_interval_seconds(void); ///< Polling interval in seconds
    int get_milliseconds_per_second(void);  ///< Milliseconds per second
    int get_thread_success_code(void);      ///< Thread success code

    // ==================== Error Message Constants ====================
    const char *get_socket_creation_error_message(void);      ///< Socket creation error message
    const char *get_socket_option_error_message(void);        ///< Socket option error message
    const char *get_socket_bind_error_message(void);          ///< Socket bind error message
    const char *get_socket_listen_error_message(void);        ///< Socket listen error message
    const char *get_thread_creation_error_message(void);      ///< Thread creation error message
    const char *get_null_config_error_message(void);          ///< Null config error message
    const char *get_invalid_port_error_message(void);         ///< Invalid port error message
    const char *get_invalid_client_count_error_message(void); ///< Invalid client count error message
    const char *get_null_parameter_error_message(void);       ///< Null parameter error message

    const char *get_server_version_string(void);    ///< Server version string
    const char *get_server_build_info_string(void); ///< Server build info string
    const char *get_empty_string(void);             ///< Empty string constant

    // ==================== Configuration Defaults ====================
    size_t get_default_max_memory(void);          ///< Default max memory
    server_mode_t get_default_server_mode(void);  ///< Default server mode
    const char *get_default_bind_address(void);   ///< Default bind address
    const char *get_default_data_directory(void); ///< Default data directory
    bool get_default_persistence_enabled(void);   ///< Default persistence enabled
    int get_default_persistence_interval(void);   ///< Default persistence interval

    // ==================== Utility Functions ====================
    double get_server_uptime_seconds(const server_instance_t *server); ///< Calculate server uptime

#ifdef __cplusplus
}
#endif