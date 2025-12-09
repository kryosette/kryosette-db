/**
 * @file main.c
 * @brief Kryocache CLI Client - Command Line Interface for Kryocache Server
 *
 * This module provides a Redis-like command-line interface for interacting
 * with Kryocache in-memory cache server. It supports all basic cache operations
 * with comprehensive error handling and user-friendly output.
 */

#include "/Users/dimaeremin/kryosette-db/kryocache/src/core/client/include/client.h"
#include "/Users/dimaeremin/kryosette-db/kryocache/src/core/client/include/constants.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include "/Users/dimaeremin/kryosette-db/kryocache/white_list/client/white_list_client.h"
#include <inttypes.h>

// ==================== CLI Constants and Configuration ====================

/**
 * @defgroup cli_constants CLI Constants
 * @{
 */

#define MAX_KEY_LENGTH 256
#define MAX_VALUE_LENGTH 1024
#define MAX_RESPONSE_BUFFER 4096
#define DEFAULT_TIMEOUT_MS 5000

/** Application return codes */
typedef enum
{
    APP_SUCCESS = 0,
    APP_ERROR_USAGE = 1,
    APP_ERROR_CONNECTION = 2,
    APP_ERROR_OPERATION = 3,
    APP_ERROR_MEMORY = 4
} app_result_t;

/** @} */

// ==================== Utility Functions ====================

/**
 * @brief Print usage information
 *
 * USER EXPERIENCE PRINCIPLE
 *
 * Command-line tools must provide clear, concise usage information that:
 * - Explains all available commands and options
 * - Shows practical examples
 * - Uses consistent formatting
 * - Highlights required vs optional parameters
 */
static void print_usage(const char *program_name)
{
    printf("Kryocache CLI Client - High-performance in-memory cache client\n\n");
    printf("Usage: %s [OPTIONS] COMMAND [ARGUMENTS]\n\n", program_name);

    printf("Options:\n");
    printf("  -h, --host HOST        Server hostname (default: %s)\n", get_client_default_host());
    printf("  -p, --port PORT        Server port (default: %u)\n", get_client_default_port());
    printf("  -t, --timeout MS       Operation timeout in milliseconds (default: %u)\n", get_client_default_timeout());
    printf("  -v, --verbose          Enable verbose output\n");
    printf("  --help                 Show this help message\n\n");

    printf("Commands:\n");
    printf("  set KEY VALUE          Store key-value pair in cache\n");
    printf("  get KEY                Retrieve value for specified key\n");
    printf("  delete KEY             Remove key from cache\n");
    printf("  exists KEY             Check if key exists in cache\n");
    printf("  flush                  Remove all keys from cache\n");
    printf("  ping                   Test server connection\n");
    printf("  stats                  Show client statistics\n");
    printf("  status                 Show connection status\n\n");

    printf("Examples:\n");
    printf("  %s set username john_doe\n", program_name);
    printf("  %s get username\n", program_name);
    printf("  %s -h 192.168.1.100 -p 6898 set counter 42\n", program_name);
    printf("  %s --verbose ping\n", program_name);
}

/**
 * @brief Print connection information
 *
 * OBSERVABILITY PRINCIPLE
 *
 * CLI tools should provide visibility into:
 * - Connection parameters and status
 * - Operation results and timing
 * - Error conditions with context
 * - Performance metrics when relevant
 */
static void print_connection_info(const client_instance_t *client, bool verbose)
{
    if (!verbose)
        return;

    const client_config_t *config = client_get_config(client);
    client_status_t status = client_get_status(client);

    printf("🔗 Connection Information:\n");
    printf("   Host: %s\n", config->host);
    printf("   Port: %u\n", config->port);
    printf("   Status: %s\n",
           status == CLIENT_STATUS_CONNECTED ? "Connected" : status == CLIENT_STATUS_CONNECTING ? "Connecting"
                                                         : status == CLIENT_STATUS_DISCONNECTED ? "Disconnected"
                                                                                                : "Error");
    printf("   Timeout: %ums\n", config->timeout_ms);
    printf("   Auto-reconnect: %s\n", config->auto_reconnect ? "Yes" : "No");
    printf("\n");
}

/**
 * @brief Print operation statistics
 *
 * PERFORMANCE MONITORING PRINCIPLE
 *
 * Statistics provide valuable insights for:
 * - Performance troubleshooting
 * - Capacity planning
 * - Usage patterns analysis
 * - Operational health monitoring
 */
static void print_statistics(const client_instance_t *client)
{
    client_stats_t stats;
    if (client_get_stats(client, &stats))
    {
        printf("📊 Client Statistics:\n");
        printf("   Operations Total: %llu\n", (unsigned long long)stats.operations_total);
        printf("   Operations Failed: %llu\n", (unsigned long long)stats.operations_failed);
        printf("   Success Rate: %.1f%%\n",
               stats.operations_total > 0 ? (100.0 * (stats.operations_total - stats.operations_failed) / stats.operations_total) : 0.0);
        printf("   Bytes Sent: %llu\n", (unsigned long long)stats.bytes_sent);
        printf("   Bytes Received: %llu\n", (unsigned long long)stats.bytes_received);
        printf("   Reconnect Count: %u\n", stats.reconnect_count);
        printf("   Connection Time: %.1f seconds\n", stats.connection_time_seconds);
    }
}

/**
 * @brief Print operation result with appropriate formatting
 *
 * USER FEEDBACK PRINCIPLE
 *
 * Command results should be:
 * - Clearly indicated as success/failure
 * - Human-readable with appropriate context
 * - Consistent across different operations
 * - Machine-parsable when needed (for scripting)
 */
static void print_result(client_result_t result, const char *operation,
                         const char *additional_info, bool verbose)
{
    const char *status_emoji = (result == CLIENT_SUCCESS) ? "✅" : "❌";

    if (verbose)
    {
        printf("%s %s: %s", status_emoji, operation, client_result_to_string(result));
        if (additional_info && additional_info[0] != '\0')
        {
            printf(" (%s)", additional_info);
        }
        printf("\n");
    }
    else
    {
        if (result != CLIENT_SUCCESS || additional_info)
        {
            printf("%s\n", additional_info ? additional_info : client_result_to_string(result));
        }
    }
}

// ==================== Command Handlers ====================

/**
 * @brief Handle SET command
 *
 * DATA STORAGE SAFETY PRINCIPLE
 *
 * Key-value storage operations must:
 * - Validate input sizes to prevent resource exhaustion
 * - Handle encoding/formatting consistently
 * - Provide clear feedback on storage results
 * - Maintain data integrity during network failures
 */
static app_result_t handle_set_command(client_instance_t *client,
                                       int argc, char *argv[],
                                       bool verbose)
{
    if (argc < 2)
    {
        fprintf(stderr, "Error: SET command requires KEY and VALUE arguments\n");
        return APP_ERROR_USAGE;
    }

    const char *key = argv[0];
    const char *value = argv[1];

    if (strlen(key) > get_client_max_key_length())
    {
        fprintf(stderr, "Error: Key too long (max %u characters)\n", get_client_max_key_length());
        return APP_ERROR_USAGE;
    }

    if (strlen(value) > get_client_max_value_length())
    {
        fprintf(stderr, "Error: Value too long (max %u characters)\n", get_client_max_value_length());
        return APP_ERROR_USAGE;
    }

    print_connection_info(client, verbose);

    client_result_t result = client_set(client, key, value);
    print_result(result, "SET", verbose ? "Stored key-value pair" : NULL, verbose);

    return (result == CLIENT_SUCCESS) ? APP_SUCCESS : APP_ERROR_OPERATION;
}

// static app_result_t handle_set_command(client_instance_t *client,
//                                        int argc, char *argv[],
//                                        bool verbose) {
//     if (argc < 3) {
//         printf("Args < 3")
//     }

//     const char *key = argv[0];
//     const char *value1 = argv[1];
//     const char *value2 = argv[2];

//     if (strlen(key) > get_client_max_key_length())
//     {
//         fprintf(stderr, "Error: Key too long (max %u characters)\n", get_client_max_key_length());
//         return APP_ERROR_USAGE;
//     }

//     if (strlen(value) > get_client_max_value_length())
//     {
//         fprintf(stderr, "Error: Value too long (max %u characters)\n", get_client_max_value_length());
//         return APP_ERROR_USAGE;
//     }

//     print_connection_info();

// client_result_t result = client_set(client, key, value);
//    print_result(result, "SET", verbose ? "Stored key-value pair" : NULL, verbose);

//    return (result == CLIENT_SUCCESS) ? APP_SUCCESS : APP_ERROR_OPERATION;
// }

/**
 * @brief Handle GET command
 *
 * DATA RETRIEVAL RELIABILITY PRINCIPLE
 *
 * Data retrieval must handle:
 * - Key existence verification
 * - Value size validation and truncation
 * - Network timeouts gracefully
 * - Missing keys with appropriate messaging
 */
static app_result_t handle_get_command(client_instance_t *client,
                                       int argc, char *argv[],
                                       bool verbose)
{
    if (argc < 1)
    {
        fprintf(stderr, "Error: GET command requires KEY argument\n");
        return APP_ERROR_USAGE;
    }

    const char *key = argv[0];

    if (strlen(key) > get_client_max_key_length())
    {
        fprintf(stderr, "Error: Key too long (max %u characters)\n", get_client_max_key_length());
        return APP_ERROR_USAGE;
    }

    print_connection_info(client, verbose);

    char value_buffer[MAX_VALUE_LENGTH];
    client_result_t result = client_get(client, key, value_buffer, sizeof(value_buffer));

    if (result == CLIENT_SUCCESS)
    {
        if (verbose)
        {
            printf("✅ GET: Retrieved value for key '%s'\n", key);
            printf("Value: %s\n", value_buffer);
        }
        else
        {
            printf("%s\n", value_buffer);
        }
    }
    else
    {
        print_result(result, "GET", verbose ? "Failed to retrieve value" : NULL, verbose);
    }

    return (result == CLIENT_SUCCESS) ? APP_SUCCESS : APP_ERROR_OPERATION;
}

/**
 * @brief Handle DELETE command
 *
 * DATA MANAGEMENT PRINCIPLE
 *
 * Data deletion operations should:
 * - Provide idempotent behavior (deleting non-existent keys succeeds)
 * - Confirm successful removal
 * - Handle concurrent modifications safely
 * - Maintain cache consistency
 */
static app_result_t handle_delete_command(client_instance_t *client,
                                          int argc, char *argv[],
                                          bool verbose)
{
    if (argc < 1)
    {
        fprintf(stderr, "Error: DELETE command requires KEY argument\n");
        return APP_ERROR_USAGE;
    }

    const char *key = argv[0];

    if (strlen(key) > get_client_max_key_length())
    {
        fprintf(stderr, "Error: Key too long (max %u characters)\n", get_client_max_key_length());
        return APP_ERROR_USAGE;
    }

    print_connection_info(client, verbose);

    client_result_t result = client_delete(client, key);
    print_result(result, "DELETE", verbose ? "Removed key from cache" : NULL, verbose);

    return (result == CLIENT_SUCCESS) ? APP_SUCCESS : APP_ERROR_OPERATION;
}

/**
 * @brief Handle EXISTS command
 *
 * CACHE STATE VERIFICATION PRINCIPLE
 *
 * Existence checks provide:
 * - Fast verification of key presence
 * - Non-destructive cache inspection
 * - Foundation for conditional operations
 * - Cache population insights
 */
static app_result_t handle_exists_command(client_instance_t *client,
                                          int argc, char *argv[],
                                          bool verbose)
{
    if (argc < 1)
    {
        fprintf(stderr, "Error: EXISTS command requires KEY argument\n");
        return APP_ERROR_USAGE;
    }

    const char *key = argv[0];

    if (strlen(key) > get_client_max_key_length())
    {
        fprintf(stderr, "Error: Key too long (max %u characters)\n", get_client_max_key_length());
        return APP_ERROR_USAGE;
    }

    print_connection_info(client, verbose);

    client_result_t result = client_exists(client, key);

    if (verbose)
    {
        const char *exists_msg = (result == CLIENT_SUCCESS) ? "Key exists" : "Key does not exist";
        print_result(result, "EXISTS", exists_msg, verbose);
    }
    else
    {
        printf("%s\n", (result == CLIENT_SUCCESS) ? "true" : "false");
    }

    return APP_SUCCESS; // EXISTS never fails, just returns existence status
}

/**
 * @brief Handle FLUSH command
 *
 * CACHE MAINTENANCE PRINCIPLE
 *
 * Cache flushing operations:
 * - Provide complete cache reset capability
 * - Should be used cautiously in production
 * - Offer performance testing utility
 * - Support cache recovery scenarios
 */
static app_result_t handle_flush_command(client_instance_t *client,
                                         bool verbose)
{
    print_connection_info(client, verbose);

    client_result_t result = client_flush(client);
    print_result(result, "FLUSH", verbose ? "Cleared all cache entries" : NULL, verbose);

    return (result == CLIENT_SUCCESS) ? APP_SUCCESS : APP_ERROR_OPERATION;
}

/**
 * @brief Handle PING command
 *
 * CONNECTIVITY TESTING PRINCIPLE
 *
 * Ping operations serve as:
 * - Basic connectivity verification
 * - Round-trip time measurement
 * - Server health checking
 * - Network troubleshooting tool
 */
static app_result_t handle_ping_command(client_instance_t *client, bool verbose)
{
    print_connection_info(client, verbose);

    client_result_t result = client_ping(client);
    print_result(result, "PING", verbose ? "Server responded" : NULL, verbose);

    return (result == CLIENT_SUCCESS) ? APP_SUCCESS : APP_ERROR_OPERATION;
}

/**
 * @brief Handle STATS command
 *
 * PERFORMANCE OBSERVABILITY PRINCIPLE
 *
 * Statistics display provides:
 * - Operational health monitoring
 * - Performance trend analysis
 * - Capacity planning data
 * - Troubleshooting insights
 */
static app_result_t handle_stats_command(client_instance_t *client, bool verbose)
{
    print_connection_info(client, verbose);

    client_stats_t stats;
    if (client_get_stats(client, &stats))
    {
        print_statistics(client);
        return APP_SUCCESS;
    }
    else
    {
        fprintf(stderr, "Error: Failed to retrieve statistics\n");
        return APP_ERROR_OPERATION;
    }
}

/**
 * @brief Handle STATUS command
 *
 * CONNECTION MONITORING PRINCIPLE
 *
 * Status information helps with:
 * - Connection state verification
 * - Configuration validation
 * - Troubleshooting connectivity issues
 * - Monitoring client health
 */
static app_result_t handle_status_command(client_instance_t *client, bool verbose)
{
    client_status_t status = client_get_status(client);
    const client_config_t *config = client_get_config(client);
    const char *last_error = client_get_last_error(client);

    printf("🔄 Client Status:\n");
    printf("   Connection: %s\n",
           status == CLIENT_STATUS_CONNECTED ? "Connected" : status == CLIENT_STATUS_CONNECTING ? "Connecting"
                                                         : status == CLIENT_STATUS_DISCONNECTED ? "Disconnected"
                                                                                                : "Error");
    printf("   Server: %s:%u\n", config->host, config->port);
    printf("   Timeout: %ums\n", config->timeout_ms);

    if (status == CLIENT_STATUS_ERROR || (verbose && last_error[0] != '\0'))
    {
        printf("   Last Error: %s\n", last_error);
    }

    if (verbose)
    {
        print_statistics(client);
    }

    return APP_SUCCESS;
}

// ==================== Main Application Logic ====================

/**
 * @brief Main application entry point
 *
 * COMMAND-LINE APPLICATION DESIGN PRINCIPLE
 *
 * Well-designed CLI applications should:
 * - Parse arguments consistently and helpfully
 * - Provide clear error messages
 * - Handle both interactive and scripted use
 * - Clean up resources properly
 * - Return meaningful exit codes
 */
int main(int argc, char *argv[])
{
    // Default configuration
    client_config_t config = client_config_default();
    bool verbose = false;
    const char *command = NULL;
    char **command_args = NULL;
    int command_argc = 0;

    // Parse command line options
    static struct option long_options[] = {
        {"host", required_argument, 0, 'h'},
        {"port", required_argument, 0, 'p'},
        {"timeout", required_argument, 0, 't'},
        {"verbose", no_argument, 0, 'v'},
        {"help", no_argument, 0, 0},
        {0, 0, 0, 0}};

    int opt;
    int option_index = 0;

    /*
    int getopt_long(int argc, char * const argv[],
           const char *optstring,
           const struct option *longopts, int *longindex);

    The getopt() function parses the command-line arguments. 
    Its arguments argc and argv are the argument count and array as passed to the main() function on program invocation. 
    An element of argv that starts with '-' (and is not exactly "-" or "--") is an option element. 
    The characters of this element (aside from the initial '-') are option characters. 
    If getopt() is called repeatedly, it returns successively each of the option characters from each of the option elements.
    */
    while ((opt = getopt_long(argc, argv, "h:p:t:v", long_options, &option_index)) != -1)
    {
        switch (opt)
        {
        case 'h':
            config.host = optarg;
            break;
        case 'p':
            config.port = (uint32_t)atoi(optarg);
            break;
        case 't':
            config.timeout_ms = (uint32_t)atoi(optarg);
            break;
        case 'v':
            verbose = true;
            break;
        case 0: // --help
            print_usage(argv[0]);
            return APP_SUCCESS;
        default:
            print_usage(argv[0]);
            return APP_ERROR_USAGE;
        }
    }

    // Extract command and arguments
    if (optind < argc)
    {
        command = argv[optind];
        command_args = &argv[optind + 1];
        command_argc = argc - optind - 1;
    }
    else
    {
        fprintf(stderr, "Error: No command specified\n\n");
        print_usage(argv[0]);
        return APP_ERROR_USAGE;
    }

    // Validate configuration
    char error_buffer[256];
    if (!client_config_validate(&config, error_buffer, sizeof(error_buffer)))
    {
        fprintf(stderr, "Configuration error: %s\n", error_buffer);
        return APP_ERROR_USAGE;
    }

    /*
    RESOURCE MANAGEMENT PRINCIPLE

    All acquired resources must be properly released:
    - Client instances created must be destroyed
    - Network connections must be closed
    - Memory allocations must be freed
    - File descriptors must be closed

    This prevents resource leaks and ensures clean shutdown.
    */
    client_instance_t *client = client_init(&config, 0);
    if (client == NULL)
    {
        fprintf(stderr, "Error: Failed to initialize client\n");
        return APP_ERROR_MEMORY;
    }

    app_result_t app_result = APP_SUCCESS;

    // Dispatch to appropriate command handler
    if (strcmp(command, "set") == 0)
    {
        app_result = handle_set_command(client, command_argc, command_args, verbose);
    }
    else if (strcmp(command, "get") == 0)
    {
        app_result = handle_get_command(client, command_argc, command_args, verbose);
    }
    else if (strcmp(command, "delete") == 0)
    {
        app_result = handle_delete_command(client, command_argc, command_args, verbose);
    }
    else if (strcmp(command, "exists") == 0)
    {
        app_result = handle_exists_command(client, command_argc, command_args, verbose);
    }
    else if (strcmp(command, "flush") == 0)
    {
        app_result = handle_flush_command(client, verbose);
    }
    else if (strcmp(command, "ping") == 0)
    {
        app_result = handle_ping_command(client, verbose);
    }
    else if (strcmp(command, "stats") == 0)
    {
        app_result = handle_stats_command(client, verbose);
    }
    else if (strcmp(command, "status") == 0)
    {
        app_result = handle_status_command(client, verbose);
    }
    else
    {
        fprintf(stderr, "Error: Unknown command '%s'\n\n", command);
        print_usage(argv[0]);
        app_result = APP_ERROR_USAGE;
    }

    // Cleanup
    client_destroy(client);

    return app_result;
}