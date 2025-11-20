/**
 * @file client_constants.h
 * @brief Client constants definition header
 */
#pragma once

#include <stdint.h>
#include <stddef.h>
#include "client.h"

#ifdef __cplusplus
extern "C"
{
#endif

    // ==================== Client Configuration Defaults ====================
    client_config_t client_config_default(void); ///< Create default client configuration

    // ==================== Client Default Values ====================
    const char *get_client_default_host(void); ///< Default server host
    uint32_t get_client_default_port(void);    ///< Default server port
    uint32_t get_client_default_timeout(void); ///< Default operation timeout
    uint32_t get_client_max_retries(void);     ///< Maximum connection retries
    bool get_client_auto_reconnect(void);      ///< Default auto-reconnect setting
    uint32_t get_max_command_length(void);     ///< Maximum command length

    // ==================== Network Constants ====================
    uint32_t get_client_buffer_size(void);      ///< I/O buffer size
    uint32_t get_client_max_key_length(void);   ///< Maximum key length
    uint32_t get_client_max_value_length(void); ///< Maximum value length

    // ==================== Protocol Constants ====================
    uint8_t get_client_protocol_version(void);       ///< Protocol version
    const char *get_client_protocol_delimiter(void); ///< Protocol delimiter

    // ==================== Client Status Constants ====================
    client_status_t get_initial_client_status(void); ///< Initial client status

    // ==================== Error Messages ====================
    const char *get_client_error_connection(void); ///< Connection error message
    const char *get_client_error_timeout(void);    ///< Timeout error message
    const char *get_client_error_protocol(void);   ///< Protocol error message
    const char *get_client_error_memory(void);     ///< Memory error message

    // ==================== Test Constants ====================
    const char *get_test_client_host(void); ///< Test server host
    uint32_t get_test_client_port(void);    ///< Test server port
    uint32_t get_test_client_timeout(void); ///< Test timeout

#ifdef __cplusplus
}
#endif