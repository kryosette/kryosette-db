/**
 * @file client_constants.c
 * @brief Client constants implementation
 */

#include "/mnt/c/Users/dmako/kryosette/kryosette-db/kryocache/src/core/client/include/constants.h"
#include <string.h>

// ==================== Client Default Values ====================

static const char *CLIENT_DEFAULT_HOST = "::1";
static const uint32_t CLIENT_DEFAULT_PORT = 6898;
static const uint32_t CLIENT_DEFAULT_TIMEOUT = 5000; // 5 seconds
static const uint32_t CLIENT_MAX_RETRIES = 3;
static const bool CLIENT_AUTO_RECONNECT = true;
static const uint32_t MAX_COMMAND_LENGTH = 4096;

// ==================== Network Constants ====================

static const uint32_t CLIENT_BUFFER_SIZE = 4096;
static const uint32_t CLIENT_MAX_KEY_LENGTH = 256;
static const uint32_t CLIENT_MAX_VALUE_LENGTH = 1048576; // 1MB

// ==================== Protocol Constants ====================

static const uint8_t CLIENT_PROTOCOL_VERSION = 0x01;
static const char *CLIENT_PROTOCOL_DELIMITER = "\r\n";

// ==================== Client Status Constants ====================

static const client_status_t INITIAL_CLIENT_STATUS = CLIENT_STATUS_DISCONNECTED;

// ==================== Error Messages ====================

static const char *CLIENT_ERRMSG_CONNECTION = "Connection failed";
static const char *CLIENT_ERRMSG_TIMEOUT = "Operation timeout";
static const char *CLIENT_ERRMSG_PROTOCOL = "Protocol error";
static const char *CLIENT_ERRMSG_MEMORY = "Memory allocation failed";

// ==================== Test Constants ====================

static const char *TEST_CLIENT_HOST = "localhost";
static const uint32_t TEST_CLIENT_PORT = 6898;
static const uint32_t TEST_CLIENT_TIMEOUT = 1000;

// ==================== Response Size Constants ====================

static const size_t MAX_RESPONSE_SIZE = 1048576; // 1MB

// ==================== Client Configuration Default Implementation ====================

// client_config_t client_config_default(void)
// {
//     client_config_t config;
//     config.host = get_client_default_host();
//     config.port = get_client_default_port();
//     config.timeout_ms = get_client_default_timeout();
//     config.max_retries = get_client_max_retries();
//     config.auto_reconnect = get_client_auto_reconnect();
//     return config;
// }

// ==================== Client Default Values Getters ====================

const char *get_client_default_host(void) { return CLIENT_DEFAULT_HOST; }
uint32_t get_client_default_port(void) { return CLIENT_DEFAULT_PORT; }
uint32_t get_client_default_timeout(void) { return CLIENT_DEFAULT_TIMEOUT; }
uint32_t get_client_max_retries(void) { return CLIENT_MAX_RETRIES; }
bool get_client_auto_reconnect(void) { return CLIENT_AUTO_RECONNECT; }
uint32_t get_max_command_length(void) { return MAX_COMMAND_LENGTH; }

// ==================== Network Constants Getters ====================

uint32_t get_client_buffer_size(void)
{
    return CLIENT_BUFFER_SIZE;
}
uint32_t get_client_max_key_length(void) { return CLIENT_MAX_KEY_LENGTH; }
uint32_t get_client_max_value_length(void) { return CLIENT_MAX_VALUE_LENGTH; }

// ==================== Protocol Constants Getters ====================

uint8_t get_client_protocol_version(void) { return CLIENT_PROTOCOL_VERSION; }
const char *get_client_protocol_delimiter(void) { return CLIENT_PROTOCOL_DELIMITER; }

// ==================== Client Status Constants Getters ====================

client_status_t get_initial_client_status(void) { return INITIAL_CLIENT_STATUS; }

// ==================== Error Messages Getters ====================

// ИЗМЕНЕНО: обновлены имена функций для соответствия новым константам
const char *get_client_error_connection(void) { return CLIENT_ERRMSG_CONNECTION; }
const char *get_client_error_timeout(void) { return CLIENT_ERRMSG_TIMEOUT; }
const char *get_client_error_protocol(void) { return CLIENT_ERRMSG_PROTOCOL; }
const char *get_client_error_memory(void) { return CLIENT_ERRMSG_MEMORY; }

// ==================== Test Constants Getters ====================

const char *get_test_client_host(void) { return TEST_CLIENT_HOST; }
uint32_t get_test_client_port(void) { return TEST_CLIENT_PORT; }
uint32_t get_test_client_timeout(void) { return TEST_CLIENT_TIMEOUT; }

// ==================== Response Size Getters ====================

size_t get_max_response_size(void) { return MAX_RESPONSE_SIZE; }