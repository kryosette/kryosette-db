/**
 * @file constants.c
 * @brief Server constants implementation
 *
 * This file provides the concrete implementation of all server-related constants
 * and their corresponding accessor functions.
 */

#include "/mnt/c/Users/dmako/kryosette/kryosette-db/kryocache/src/core/server/include/constants.h"
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
static const size_t MAX_SAFE_CLIENT_COUNT = 100000;
static const size_t DEFAULT_CLIENT_COUNT = 1000;

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

// ==================== Network Constants ====================

static const int SOCKET_PROTOCOL = 0;
static const int SOCKET_SUCCESS_CODE = 0;
static const int SOCKET_REUSEADDR_OPTION = 1;
static const int SOCKET_LEVEL = SOL_SOCKET;
static const int SOCKET_DOMAIN = AF_INET6;
static const int SOCKET_BIND_ADDRESS = INADDR_ANY;
static const int SOCKET_BACKLOG = 10;
static const int SOCKET_SHUTDOWN_MODE = SHUT_RDWR;

// ==================== Test Constants ====================

static const int TEST_CUSTOM_PORT = 9999;
static const uint32_t TEST_MAX_CLIENTS = 500;
static const size_t TEST_MAX_MEMORY = 536870912; // 512MB
static const server_mode_t TEST_SERVER_MODE = SERVER_MODE_STANDALONE;
static const char *TEST_BIND_ADDRESS = "127.0.0.1";
static const char *TEST_DATA_DIRECTORY = "/tmp/test_cache";
static const bool TEST_PERSISTENCE_ENABLED = true;
static const int TEST_PERSISTENCE_INTERVAL = 60;

static const int INVALID_PORT_NUMBER = 70000;
static const uint32_t INVALID_CLIENT_COUNT = 0;
static const uint32_t MINIMUM_PORT_NUMBER = 1024;
static const uint32_t MAXIMUM_PORT_NUMBER = 65535;
static const uint32_t MINIMUM_CLIENT_COUNT = 1;

static const int INITIAL_TEST_COUNT = 0;
static const int INITIAL_TEST_INDEX = 0;
static const int INITIAL_CONNECTION_COUNT = 0;
static const int INITIAL_COMMAND_COUNT = 0;
static const size_t INITIAL_MEMORY_USAGE = 0;
static const int STRING_START_INDEX = 0;
static const char STRING_TERMINATOR = '\0';
static const size_t ERROR_BUFFER_SIZE = 256;
static const int MINIMUM_VERSION_LENGTH = 1;
static const int MINIMUM_BUILD_INFO_LENGTH = 1;

static const int INIT_TEST_COUNT = 3;
static const int CONFIG_TEST_COUNT = 2;
static const int INFO_TEST_COUNT = 2;
static const int ADVANCED_TEST_COUNT = 2;

static const int POLLING_INTERVAL_SECONDS = 1;
static const int MILLISECONDS_PER_SECOND = 1000;
static const int THREAD_SUCCESS_CODE = 0;

// ==================== Error Message Constants ====================

static const char *SOCKET_CREATION_ERROR_MESSAGE = "Socket creation failed";
static const char *SOCKET_OPTION_ERROR_MESSAGE = "Socket option setting failed";
static const char *SOCKET_BIND_ERROR_MESSAGE = "Socket bind failed";
static const char *SOCKET_LISTEN_ERROR_MESSAGE = "Socket listen failed";
static const char *THREAD_CREATION_ERROR_MESSAGE = "Thread creation failed";
static const char *NULL_CONFIG_ERROR_MESSAGE = "Configuration is NULL";
static const char *INVALID_PORT_ERROR_MESSAGE = "Invalid port number: %d";
static const char *INVALID_CLIENT_COUNT_ERROR_MESSAGE = "Invalid client count";
static const char *NULL_PARAMETER_ERROR_MESSAGE = "NULL parameter provided";

static const char *SERVER_VERSION_STRING = "1.0.0";
static const char *SERVER_BUILD_INFO_STRING = "In-Memory Cache Server 1.0.0";
static const char *EMPTY_STRING = "";

static const size_t DEFAULT_MAX_MEMORY = 0; // Unlimited
static const server_mode_t DEFAULT_SERVER_MODE = SERVER_MODE_STANDALONE;
static const char *DEFAULT_BIND_ADDRESS = NULL; // All interfaces
static const char *DEFAULT_DATA_DIRECTORY = "./data";
static const bool DEFAULT_PERSISTENCE_ENABLED = false;
static const int DEFAULT_PERSISTENCE_INTERVAL = 300; // 5 minutes

// ==================== Server Configuration Default Implementation ====================

// static server_config_t server_config_default(void)
// {
//     server_config_t config;

//     config.port = get_server_default_port();
//     config.max_clients = get_server_max_clients();
//     config.max_memory = get_default_max_memory();
//     config.mode = get_default_server_mode();
//     config.bind_address = get_default_bind_address();
//     config.data_directory = get_default_data_directory();
//     config.persistence_enabled = get_default_persistence_enabled();
//     config.persistence_interval = get_default_persistence_interval();

//     return config;
// }

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
size_t get_max_safe_client_count(void) { return MAX_SAFE_CLIENT_COUNT; }
size_t get_default_client_count(void) { return DEFAULT_CLIENT_COUNT; }

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

// ==================== Network Constants Getters ====================

int get_socket_protocol(void) { return SOCKET_PROTOCOL; }
int get_socket_success_code(void) { return SOCKET_SUCCESS_CODE; }
int get_socket_reuseaddr_option(void) { return SOCKET_REUSEADDR_OPTION; }
int get_socket_level(void) { return SOCKET_LEVEL; }
int get_socket_domain(void) { return SOCKET_DOMAIN; }
int get_socket_bind_address(void) { return SOCKET_BIND_ADDRESS; }
int get_socket_backlog(void) { return SOCKET_BACKLOG; }
int get_socket_shutdown_mode(void) { return SOCKET_SHUTDOWN_MODE; }

// ==================== Test Constants Getters ====================

int get_test_custom_port(void) { return TEST_CUSTOM_PORT; }
uint32_t get_test_max_clients(void) { return TEST_MAX_CLIENTS; }
size_t get_test_max_memory(void) { return TEST_MAX_MEMORY; }
server_mode_t get_test_server_mode(void) { return TEST_SERVER_MODE; }
const char *get_test_bind_address(void) { return TEST_BIND_ADDRESS; }
const char *get_test_data_directory(void) { return TEST_DATA_DIRECTORY; }
bool get_test_persistence_enabled(void) { return TEST_PERSISTENCE_ENABLED; }
int get_test_persistence_interval(void) { return TEST_PERSISTENCE_INTERVAL; }

int get_invalid_port_number(void) { return INVALID_PORT_NUMBER; }
uint32_t get_invalid_client_count(void) { return INVALID_CLIENT_COUNT; }
uint32_t get_minimum_port_number(void) { return MINIMUM_PORT_NUMBER; }
uint32_t get_maximum_port_number(void) { return MAXIMUM_PORT_NUMBER; }
uint32_t get_minimum_client_count(void) { return MINIMUM_CLIENT_COUNT; }

int get_initial_test_count(void) { return INITIAL_TEST_COUNT; }
int get_initial_test_index(void) { return INITIAL_TEST_INDEX; }
int get_initial_connection_count(void) { return INITIAL_CONNECTION_COUNT; }
int get_initial_command_count(void) { return INITIAL_COMMAND_COUNT; }
size_t get_initial_memory_usage(void) { return INITIAL_MEMORY_USAGE; }
int get_string_start_index(void) { return STRING_START_INDEX; }
char get_string_terminator(void) { return STRING_TERMINATOR; }
size_t get_error_buffer_size(void) { return ERROR_BUFFER_SIZE; }
int get_minimum_version_length(void) { return MINIMUM_VERSION_LENGTH; }
int get_minimum_build_info_length(void) { return MINIMUM_BUILD_INFO_LENGTH; }

int get_init_test_count(void) { return INIT_TEST_COUNT; }
int get_config_test_count(void) { return CONFIG_TEST_COUNT; }
int get_info_test_count(void) { return INFO_TEST_COUNT; }
int get_advanced_test_count(void) { return ADVANCED_TEST_COUNT; }

int get_polling_interval_seconds(void) { return POLLING_INTERVAL_SECONDS; }
int get_milliseconds_per_second(void) { return MILLISECONDS_PER_SECOND; }
int get_thread_success_code(void) { return THREAD_SUCCESS_CODE; }

// ==================== Error Message Getters ====================

const char *get_socket_creation_error_message(void) { return SOCKET_CREATION_ERROR_MESSAGE; }
const char *get_socket_option_error_message(void) { return SOCKET_OPTION_ERROR_MESSAGE; }
const char *get_socket_bind_error_message(void) { return SOCKET_BIND_ERROR_MESSAGE; }
const char *get_socket_listen_error_message(void) { return SOCKET_LISTEN_ERROR_MESSAGE; }
const char *get_thread_creation_error_message(void) { return THREAD_CREATION_ERROR_MESSAGE; }
const char *get_null_config_error_message(void) { return NULL_CONFIG_ERROR_MESSAGE; }
const char *get_invalid_port_error_message(void) { return INVALID_PORT_ERROR_MESSAGE; }
const char *get_invalid_client_count_error_message(void) { return INVALID_CLIENT_COUNT_ERROR_MESSAGE; }
const char *get_null_parameter_error_message(void) { return NULL_PARAMETER_ERROR_MESSAGE; }

const char *get_server_version_string(void) { return SERVER_VERSION_STRING; }
const char *get_server_build_info_string(void) { return SERVER_BUILD_INFO_STRING; }
const char *get_empty_string(void) { return EMPTY_STRING; }

// ==================== Configuration Default Getters ====================

size_t get_default_max_memory(void) { return DEFAULT_MAX_MEMORY; }
server_mode_t get_default_server_mode(void) { return DEFAULT_SERVER_MODE; }
const char *get_default_bind_address(void) { return DEFAULT_BIND_ADDRESS; }
const char *get_default_data_directory(void) { return DEFAULT_DATA_DIRECTORY; }
bool get_default_persistence_enabled(void) { return DEFAULT_PERSISTENCE_ENABLED; }
int get_default_persistence_interval(void) { return DEFAULT_PERSISTENCE_INTERVAL; }

// ==================== Utility Functions ====================

double get_server_uptime_seconds(const server_instance_t *server)
{
    if (server == NULL || server->start_time == 0)
    {
        return 0.0;
    }
    return difftime(time(NULL), server->start_time);
}
