/**
 * @file constants.c
 * @brief Server constants implementation
 *
 * This file provides the concrete implementation of all server-related constants
 * and their corresponding accessor functions.
 */

#include "constants.h"
#include "server.h"
#include <pthread.h>

// ==================== Server Default Values ====================

static const uint16_t SERVER_DEFAULT_PORT = 6898;
static const uint32_t SERVER_MAX_CLIENTS = 10000;
static const size_t SERVER_BUFFER_SIZE = 4096;
static const size_t MAX_KEY_LENGTH = 256;
static const size_t MAX_VALUE_LENGTH = 1048576; // 1MB
static const size_t INITIAL_STORAGE_CAPACITY = 1024;
static const size_t INITIAL_STORAGE_SIZE = 0;

// ==================== Server Status Constants ====================

static const server_status_t INITIAL_SERVER_STATUS = SERVER_STATUS_STOPPED;
static const int SERVER_STATUS_STOPPED_CODE = 0;
static const int SERVER_STATUS_RUNNING_CODE = 1;
static const int SERVER_STATUS_SHUTTING_DOWN_CODE = 2;

// ==================== Threading Constants ====================

static const int MUTEX_SUCCESS_CODE = 0;
static const int INITIAL_SERVER_FD = -1;
static const pthread_t INITIAL_THREAD_ID = 0;

// ==================== Client Constants ====================

static const uint32_t INITIAL_CLIENT_COUNT = 0;

// ==================== Utility Constants ====================

static const char *INITIAL_ERROR_MESSAGE = "";
static const time_t INITIAL_START_TIME = 0;

// ==================== Server Error Codes ====================

static const int SERVER_ERR_SUCCESS = 0;
static const int SERVER_ERR_UNKNOWN_COMMAND = 1;
static const int SERVER_ERR_INVALID_ARGS = 2;
static const int SERVER_ERR_MEMORY_FULL = 3;
static const int SERVER_ERR_KEY_NOT_FOUND = 4;
static const int SERVER_ERR_KEY_EXISTS = 5;
static const int SERVER_ERR_UNAUTHORIZED = 6;
static const int SERVER_ERR_SYSTEM = 7;

// ==================== Protocol Constants ====================

static const uint8_t PROTOCOL_VERSION = 0x01;
static const uint8_t PROTOCOL_MAGIC_BYTE = 0xAE;
static const uint8_t PROTOCOL_HANDSHAKE_TIMEOUT = 10; // seconds

// ==================== Performance Constants ====================

static const size_t INITIAL_HASH_TABLE_SIZE = 1024;
static const size_t MAX_HASH_TABLE_SIZE = 65536;
static const uint32_t CLEANUP_INTERVAL_SECONDS = 300; // 5 minutes
static const uint32_t CLIENT_TIMEOUT_SECONDS = 3600;  // 1 hour

// ==================== Command Constants ====================

static const size_t MAX_COMMAND_LENGTH = 8192;
static const size_t MAX_ARGS_PER_COMMAND = 64;
static const uint32_t MAX_COMMAND_EXECUTION_TIME = 5000; // 5 seconds

// ==================== Server Default Values Getters ====================

uint16_t get_server_default_port(void) { return SERVER_DEFAULT_PORT; }
uint32_t get_server_max_clients(void) { return SERVER_MAX_CLIENTS; }
size_t get_server_buffer_size(void) { return SERVER_BUFFER_SIZE; }
size_t get_max_key_length(void) { return MAX_KEY_LENGTH; }
size_t get_max_value_length(void) { return MAX_VALUE_LENGTH; }
size_t get_initial_storage_capacity(void) { return INITIAL_STORAGE_CAPACITY; }
size_t get_initial_storage_size(void) { return INITIAL_STORAGE_SIZE; }

// ==================== Server Status Constants Getters ====================

server_status_t get_initial_server_status(void) { return INITIAL_SERVER_STATUS; }
int get_server_status_stopped(void) { return SERVER_STATUS_STOPPED_CODE; }
int get_server_status_running(void) { return SERVER_STATUS_RUNNING_CODE; }
int get_server_status_shutting_down(void) { return SERVER_STATUS_SHUTTING_DOWN_CODE; }

// ==================== Threading Constants Getters ====================

int get_mutex_success_code(void) { return MUTEX_SUCCESS_CODE; }
int get_initial_server_fd(void) { return INITIAL_SERVER_FD; }
pthread_t get_initial_thread_id(void) { return INITIAL_THREAD_ID; }

// ==================== Client Constants Getters ====================

uint32_t get_max_clients_count(const server_config_t *config)
{
    return config->max_clients;
}

uint32_t get_initial_client_count(void) { return INITIAL_CLIENT_COUNT; }

// ==================== Utility Constants Getters ====================

const char *get_initial_error_message(void) { return INITIAL_ERROR_MESSAGE; }
time_t get_initial_start_time(void) { return INITIAL_START_TIME; }

// ==================== Server Error Codes Getters ====================

int get_server_err_success(void) { return SERVER_ERR_SUCCESS; }
int get_server_err_unknown_command(void) { return SERVER_ERR_UNKNOWN_COMMAND; }
int get_server_err_invalid_args(void) { return SERVER_ERR_INVALID_ARGS; }
int get_server_err_memory_full(void) { return SERVER_ERR_MEMORY_FULL; }
int get_server_err_key_not_found(void) { return SERVER_ERR_KEY_NOT_FOUND; }
int get_server_err_key_exists(void) { return SERVER_ERR_KEY_EXISTS; }
int get_server_err_unauthorized(void) { return SERVER_ERR_UNAUTHORIZED; }
int get_server_err_system(void) { return SERVER_ERR_SYSTEM; }

// ==================== Protocol Constants Getters ====================

uint8_t get_protocol_version(void) { return PROTOCOL_VERSION; }
uint8_t get_protocol_magic_byte(void) { return PROTOCOL_MAGIC_BYTE; }
uint8_t get_protocol_handshake_timeout(void) { return PROTOCOL_HANDSHAKE_TIMEOUT; }

// ==================== Performance Constants Getters ====================

size_t get_initial_hash_table_size(void) { return INITIAL_HASH_TABLE_SIZE; }
size_t get_max_hash_table_size(void) { return MAX_HASH_TABLE_SIZE; }
uint32_t get_cleanup_interval_seconds(void) { return CLEANUP_INTERVAL_SECONDS; }
uint32_t get_client_timeout_seconds(void) { return CLIENT_TIMEOUT_SECONDS; }

// ==================== Command Constants Getters ====================

size_t get_max_command_length(void) { return MAX_COMMAND_LENGTH; }
size_t get_max_args_per_command(void) { return MAX_ARGS_PER_COMMAND; }
uint32_t get_max_command_execution_time(void) { return MAX_COMMAND_EXECUTION_TIME; }