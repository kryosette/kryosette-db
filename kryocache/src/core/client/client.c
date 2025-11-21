#include "/mnt/c/Users/dmako/kryosette/kryosette-db/kryocache/src/core/client/include/client.h"
#include "/mnt/c/Users/dmako/kryosette/kryosette-db/kryocache/src/core/client/include/constants.h"
#include "/mnt/c/Users/dmako/kryosette/kryosette-db/third-party/smemset/include/smemset.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>

// ==================== Internal Protocol Functions ====================

/**
 * @brief Internal function to send command to server and receive response
 *
 * PROTOCOL SAFETY PRINCIPLE
 *
 * All network operations must be atomic and protected by mutex to prevent
 * interleaved commands from multiple threads. Each operation follows the pattern:
 * 1. Acquire lock
 * 2. Send complete command
 * 3. Wait for complete response
 * 4. Release lock
 *
 * This ensures thread safety and prevents protocol desynchronization.
 */
static client_result_t client_send_command(client_instance_t *client,
                                           const char *command,
                                           char *response_buffer,
                                           size_t response_size)
{
    if (client == NULL || command == NULL || response_buffer == NULL)
    {
        return CLIENT_ERROR_CONNECTION;
    }

    // size_t MAX_RESPONSE_SIZE = get_max_response_size();
    uint32_t MAX_COMMAND_LENGTH = get_max_command_length();
    if (response_size == 0 || response_size > get_client_buffer_size())
    {
        return CLIENT_ERROR_INVALID_PARAM;
    }

    pthread_mutex_lock(&client->lock);

    if (client->status != CLIENT_STATUS_CONNECTED)
    {
        pthread_mutex_unlock(&client->lock);
        return CLIENT_ERROR_CONNECTION;
    }

    client_result_t result = CLIENT_SUCCESS;
    ssize_t bytes_sent, bytes_received;

    size_t command_len = strlen(command);
    if (command_len == 0 || command_len > MAX_COMMAND_LENGTH)
    {
        snprintf(client->last_error, sizeof(client->last_error),
                 "Invalid command length: %zu", command_len);
        result = CLIENT_ERROR_INVALID_PARAM;
        goto cleanup;
    }

    bytes_sent = send(client->sockfd, command, strlen(command), 0);
    if (bytes_sent < 0)
    {
        snprintf(client->last_error, sizeof(client->last_error),
                 "Send failed: %s", strerror(errno));
        result = CLIENT_ERROR_CONNECTION;
        goto cleanup;
    }

    client->stats.bytes_sent += bytes_sent;

    if (response_size == 0 || response_buffer == NULL)
    {
        snprintf(client->last_error, sizeof(client->last_error),
                 "Invalid response buffer");
        result = CLIENT_ERROR_INVALID_PARAM;
        goto cleanup;
    }

    /*
    ssize_t recv(int sockfd, void *buf, size_t len, int flags);
    sockfd:
The socket file descriptor from which to receive data. This is typically a socket that has been connected to a remote peer (e.g., using connect() for a client or accept() for a server).
buf:
A pointer to a buffer where the received data will be stored.
len:
The maximum number of bytes to receive, which is the size of the buffer pointed to by buf.
flags:
Optional flags that modify the behavior of recv(). Common flags include MSG_PEEK (to peek at incoming data without removing it from the receive queue) and MSG_WAITALL (to block until len bytes are received or an error occurs).
If no special behavior is needed, 0 is typically used.
    */
    bytes_received = recv(client->sockfd, response_buffer, response_size - 1, 0);
    if (bytes_received < 0)
    {
        snprintf(client->last_error, sizeof(client->last_error),
                 "Receive failed: %s", strerror(errno));
        result = CLIENT_ERROR_CONNECTION;
        goto cleanup;
    }

    response_buffer[bytes_received] = '\0'; // Null-terminate
    client->stats.bytes_received += bytes_received;
    client->last_activity = time(NULL);

cleanup:
    pthread_mutex_unlock(&client->lock);
    return result;
}

/**
 * @brief Internal function to establish TCP connection
 *
 * NETWORK RESILIENCE PRINCIPLE
 *
 * Connection establishment must handle transient failures gracefully:
 * - DNS resolution failures
 * - Connection timeouts
 * - Network unreachable
 * - Server not ready
 *
 * Automatic retries with exponential backoff provide robustness in
 * distributed systems where temporary network partitions are common.
 */
static client_result_t client_establish_connection(client_instance_t *client)
{
    // task: consider more errors to eliminate them
    if (client == NULL)
    {
        return CLIENT_ERROR_CONNECTION;
    }

    struct in6_addr ipv6_addr;
    if (inet_pton(AF_INET6, client->config)) {
      
    }
    // af_inet = ipv4; sock_stream = tcp
    client->sockfd = socket(AF_INET6, SOCK_STREAM, 0);
    if (client->sockfd < 0)
    {
        snprintf(client->last_error, sizeof(client->last_error),
                 "Socket creation failed: %s", strerror(errno));
        return CLIENT_ERROR_CONNECTION;
    }

    // Configure server address
    /*
    getaddrinfo(), freeaddrinfo(), gai_strerror():
           Since glibc 2.22:
               _POSIX_C_SOURCE >= 200112L
           glibc 2.21 and earlier:
               _POSIX_C_SOURCE
    don't use macros, using compiler flags
    gcc -D_POSIX_C_SOURCE=200112L -o program program.c
    # Makefile
    CFLAGS += -D_POSIX_C_SOURCE=200112L

    struct addrinfo {
        int ai_flags; // Flags (AI_PASSIVE, etc)
        int ai_family; // AF_INET, AF_INET6, AF_UNSPEC
        int ai_socktype; // SOCK_STREAM, SOCK_DGRAM
        int ai_protocol; // IPPROTO_TCP, etc
        socklen_t ai_addrlen; // Address length
        struct sockaddr *ai_addr; // Pointer to the address
        char *ai_canonname; // Canonical hostname
        struct addrinfo *ai_next; // Next structure in the list
    };

    The hints argument points to an addrinfo structure that specifies
       criteria for selecting the socket address structures returned in
       the list pointed to by res.
    If hints is not NULL it points to an
       addrinfo structure whose ai_family, ai_socktype, and ai_protocol
       specify criteria that limit the set of socket addresses returned
       by getaddrinfo()
    */
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    smemset(&client->server_addr, 0, sizeof(client->server_addr));
    client->server_addr.ai_family = AF_INET6;
    client->server_addr.socktype = SOCK_STREAM;
    client->server_addr.ai_port = htons(client->config.port);

    // Resolve hostname to IP address
    /*
    int inet_pton(int af, const char *restrict src, void *restrict dst);

    *a function for converting a string with an IP address to a binary format
    This function converts the character string src into a network
       address structure in the af address family, then copies the
       network address structure to dst.  The af argument must be either
       AF_INET or AF_INET6.  dst is written in network byte order.
    */
    if (inet_pton(AF_INET, client->config.host, &client->server_addr.sin_addr) <= 0)
    {
        // gethostbyname - deprecated! don't use this!
        // struct hostent *he = gethostbyname(client->config.host);
        if (he == NULL)
        {
            snprintf(client->last_error, sizeof(client->last_error),
                     "Host resolution failed for: %s", client->config.host);
            close(client->sockfd);
            client->sockfd = -1;
            return CLIENT_ERROR_CONNECTION;
        }
        memcpy(&client->server_addr.sin_addr, he->h_addr_list[0], he->h_length);
    }

    // Set socket timeout
    struct timeval timeout = 0;
    timeout.tv_sec = client->config.timeout_ms / 1000;
    timeout.tv_usec = (client->config.timeout_ms % 1000) * 1000;

    setsockopt(client->sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(client->sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    // Attempt connection with retries
    uint32_t attempt = 0;
    for (attempt = 0; attempt < client->config.max_retries; attempt++)
    {
        if (connect(client->sockfd, (struct sockaddr *)&client->server_addr,
                    sizeof(client->server_addr)) == 0)
        {
            client->status = CLIENT_STATUS_CONNECTED;
            client->connect_time = time(NULL);
            client->last_activity = client->connect_time;
            return CLIENT_SUCCESS;
        }

        if (attempt < client->config.max_retries - 1)
        {
            usleep(100000 * (1 << attempt)); // Exponential backoff
        }
    }

    snprintf(client->last_error, sizeof(client->last_error),
             "Connection failed after %d attempts: %s",
             client->config.max_retries, strerror(errno));
    close(client->sockfd);
    client->sockfd = -1;
    return CLIENT_ERROR_CONNECTION;
}

// ==================== Core Client API Implementation ====================

/*
CLIENT LIFECYCLE MANAGEMENT PRINCIPLE

Client instances follow a strict lifecycle:
1. INIT: Memory allocation and configuration
2. CONNECT: Network establishment and handshake
3. OPERATE: Command execution (thread-safe)
4. DISCONNECT: Graceful connection teardown
5. DESTROY: Resource cleanup

Each phase must handle partial failure states and ensure
no resource leaks occur during error recovery.
*/

client_instance_t *client_init_default(void)
{
    static client_config_t DEFAULT_CONFIG;
    static int initialized = 0;

    if (!initialized)
    {
        DEFAULT_CONFIG.host = get_client_default_host();
        DEFAULT_CONFIG.port = get_client_default_port();
        DEFAULT_CONFIG.timeout_ms = get_client_default_timeout();
        DEFAULT_CONFIG.max_retries = get_client_max_retries();
        DEFAULT_CONFIG.auto_reconnect = get_client_auto_reconnect();
        initialized = 1;
    }

    return client_init(&DEFAULT_CONFIG);
}

client_instance_t *client_init(const client_config_t *config)
{
    if (config == NULL)
    {
        return NULL;
    }

    /*
    MEMORY SAFETY: ZERO-INITIALIZATION
    Using calloc ensures all pointer fields start as NULL and numeric fields as 0.
    This prevents accessing uninitialized memory and simplifies cleanup logic.
    */
    client_instance_t *client = (client_instance_t *)calloc(1, sizeof(client_instance_t));
    if (client == NULL)
    {
        return NULL;
    }

    // Copy configuration (shallow copy for now)
    client->config = *config;
    client->status = get_initial_client_status();
    client->sockfd = -1; // Invalid socket descriptor

    /*
    THREAD SAFETY: OPERATION ISOLATION

    The client mutex protects all network operations to ensure:
    - Only one command executes at a time
    - Response boundaries are preserved
    - Statistics updates are atomic
    - Connection state changes are synchronized
    */
    if (pthread_mutex_init(&client->lock, NULL) != 0)
    {
        free(client);
        return NULL;
    }

    // Initialize statistics
    client->stats.operations_total = 0;
    client->stats.operations_failed = 0;
    client->stats.bytes_sent = 0;
    client->stats.bytes_received = 0;
    client->stats.reconnect_count = 0;
    client->stats.connection_time_seconds = 0.0;

    // Initialize error buffer and timestamps
    client->last_error[0] = '\0';
    client->connect_time = 0;
    client->last_activity = 0;

    return client;
}

client_result_t client_connect(client_instance_t *client)
{
    if (client == NULL)
    {
        return CLIENT_ERROR_CONNECTION;
    }

    pthread_mutex_lock(&client->lock);

    if (client->status == CLIENT_STATUS_CONNECTED)
    {
        pthread_mutex_unlock(&client->lock);
        return CLIENT_SUCCESS; // Already connected
    }

    client->status = CLIENT_STATUS_CONNECTING;
    pthread_mutex_unlock(&client->lock);

    client_result_t result = client_establish_connection(client);

    pthread_mutex_lock(&client->lock);
    if (result != CLIENT_SUCCESS)
    {
        client->status = CLIENT_STATUS_ERROR;
        client->stats.operations_failed++;
    }
    pthread_mutex_unlock(&client->lock);

    return result;
}

client_result_t client_disconnect(client_instance_t *client)
{
    if (client == NULL)
    {
        return CLIENT_ERROR_CONNECTION;
    }

    pthread_mutex_lock(&client->lock);

    if (client->status != CLIENT_STATUS_CONNECTED)
    {
        pthread_mutex_unlock(&client->lock);
        return CLIENT_SUCCESS; // Already disconnected
    }

    if (client->sockfd >= 0)
    {
        close(client->sockfd);
        client->sockfd = -1;
    }

    // Update connection time statistics
    if (client->connect_time > 0)
    {
        client->stats.connection_time_seconds += difftime(time(NULL), client->connect_time);
        client->connect_time = 0;
    }

    client->status = CLIENT_STATUS_DISCONNECTED;
    pthread_mutex_unlock(&client->lock);

    return CLIENT_SUCCESS;
}

void client_destroy(client_instance_t *client)
{
    if (client == NULL)
    {
        return;
    }

    /*
    CLEANUP SAFETY: IDEMPOTENT DESTRUCTION

    Destruction must be safe to call multiple times and handle
    partial initialization states. The order of cleanup ensures
    resources are released in reverse allocation order.
    */

    // Gracefully disconnect if connected
    if (client->status == CLIENT_STATUS_CONNECTED)
    {
        client_disconnect(client);
    }

    // Destroy synchronization primitives
    pthread_mutex_destroy(&client->lock);

    // Free allocated resources
    // Note: config.host is not duplicated, so no free needed

    free(client);
}

// ==================== Client Operations API Implementation ====================

/*
PROTOCOL DESIGN PRINCIPLE

The client uses a simple text-based protocol similar to Redis:
- Commands: "SET key value", "GET key", "DELETE key"
- Responses: "OK", "VALUE", "ERROR message"
- Delimiters: "\r\n" for command termination

This design provides:
- Human readability for debugging
- Simple parsing logic
- Easy extensibility for new commands
- Compatibility with telnet testing
*/

client_result_t client_set(client_instance_t *client, const char *key, const char *value)
{
    if (client == NULL || key == NULL || value == NULL)
    {
        return CLIENT_ERROR_CONNECTION;
    }

    /*
    INPUT VALIDATION PRINCIPLE

    All user inputs must be validated before processing:
    - Key length limits prevent buffer overflows
    - Value length limits prevent resource exhaustion
    - NULL checks prevent segmentation faults

    Early validation provides clear error messages and
    prevents invalid state propagation.
    */
    if (strlen(key) > get_client_max_key_length())
    {
        snprintf(client->last_error, sizeof(client->last_error),
                 "Key too long: %zu bytes (max: %u)", strlen(key), get_client_max_key_length());
        return CLIENT_ERROR_PROTOCOL;
    }

    if (strlen(value) > get_client_max_value_length())
    {
        snprintf(client->last_error, sizeof(client->last_error),
                 "Value too long: %zu bytes (max: %u)", strlen(value), get_client_max_value_length());
        return CLIENT_ERROR_PROTOCOL;
    }

    // Ensure connection
    client_result_t conn_result = client_connect(client);
    if (conn_result != CLIENT_SUCCESS)
    {
        return conn_result;
    }

    // Build command: "SET key value\r\n"
    char command[get_client_max_key_length() + get_client_max_value_length() + 32];
    snprintf(command, sizeof(command), "SET %s %s\r\n", key, value);

    char response[get_client_buffer_size()];
    client_result_t result = client_send_command(client, command, response, sizeof(response));

    pthread_mutex_lock(&client->lock);
    client->stats.operations_total++;
    if (result != CLIENT_SUCCESS)
    {
        client->stats.operations_failed++;
    }
    pthread_mutex_unlock(&client->lock);

    return result;
}

client_result_t client_get(client_instance_t *client, const char *key, char *value_buffer, size_t buffer_size)
{
    if (client == NULL || key == NULL || value_buffer == NULL || buffer_size == 0)
    {
        return CLIENT_ERROR_CONNECTION;
    }

    if (strlen(key) > get_client_max_key_length())
    {
        snprintf(client->last_error, sizeof(client->last_error),
                 "Key too long: %zu bytes (max: %u)", strlen(key), get_client_max_key_length());
        return CLIENT_ERROR_PROTOCOL;
    }

    // Ensure connection
    client_result_t conn_result = client_connect(client);
    if (conn_result != CLIENT_SUCCESS)
    {
        return conn_result;
    }

    // Build command: "GET key\r\n"
    char command[get_client_max_key_length() + 32];
    snprintf(command, sizeof(command), "GET %s\r\n", key);

    char response[get_client_buffer_size()];
    client_result_t result = client_send_command(client, command, response, sizeof(response));

    pthread_mutex_lock(&client->lock);
    client->stats.operations_total++;

    if (result == CLIENT_SUCCESS)
    {
        // Copy value to user buffer (safe bounded copy)
        strncpy(value_buffer, response, buffer_size - 1);
        value_buffer[buffer_size - 1] = '\0'; // Ensure null termination
    }
    else
    {
        client->stats.operations_failed++;
    }
    pthread_mutex_unlock(&client->lock);

    return result;
}

client_result_t client_delete(client_instance_t *client, const char *key)
{
    if (client == NULL || key == NULL)
    {
        return CLIENT_ERROR_CONNECTION;
    }

    if (strlen(key) > get_client_max_key_length())
    {
        snprintf(client->last_error, sizeof(client->last_error),
                 "Key too long: %zu bytes (max: %u)", strlen(key), get_client_max_key_length());
        return CLIENT_ERROR_PROTOCOL;
    }

    client_result_t conn_result = client_connect(client);
    if (conn_result != CLIENT_SUCCESS)
    {
        return conn_result;
    }

    char command[get_client_max_key_length() + 32];
    snprintf(command, sizeof(command), "DELETE %s\r\n", key);

    char response[get_client_buffer_size()];
    client_result_t result = client_send_command(client, command, response, sizeof(response));

    pthread_mutex_lock(&client->lock);
    client->stats.operations_total++;
    if (result != CLIENT_SUCCESS)
    {
        client->stats.operations_failed++;
    }
    pthread_mutex_unlock(&client->lock);

    return result;
}

client_result_t client_exists(client_instance_t *client, const char *key)
{
    if (client == NULL || key == NULL)
    {
        return CLIENT_ERROR_CONNECTION;
    }

    if (strlen(key) > get_client_max_key_length())
    {
        snprintf(client->last_error, sizeof(client->last_error),
                 "Key too long: %zu bytes (max: %u)", strlen(key), get_client_max_key_length());
        return CLIENT_ERROR_PROTOCOL;
    }

    client_result_t conn_result = client_connect(client);
    if (conn_result != CLIENT_SUCCESS)
    {
        return conn_result;
    }

    char command[get_client_max_key_length() + 32];
    snprintf(command, sizeof(command), "EXISTS %s\r\n", key);

    char response[get_client_buffer_size()];
    client_result_t result = client_send_command(client, command, response, sizeof(response));

    pthread_mutex_lock(&client->lock);
    client->stats.operations_total++;
    if (result != CLIENT_SUCCESS)
    {
        client->stats.operations_failed++;
    }
    pthread_mutex_unlock(&client->lock);

    return result;
}

client_result_t client_flush(client_instance_t *client)
{
    if (client == NULL)
    {
        return CLIENT_ERROR_CONNECTION;
    }

    client_result_t conn_result = client_connect(client);
    if (conn_result != CLIENT_SUCCESS)
    {
        return conn_result;
    }

    char response[get_client_buffer_size()];
    client_result_t result = client_send_command(client, "FLUSH\r\n", response, sizeof(response));

    pthread_mutex_lock(&client->lock);
    client->stats.operations_total++;
    if (result != CLIENT_SUCCESS)
    {
        client->stats.operations_failed++;
    }
    pthread_mutex_unlock(&client->lock);

    return result;
}

client_result_t client_ping(client_instance_t *client)
{
    if (client == NULL)
    {
        return CLIENT_ERROR_CONNECTION;
    }

    client_result_t conn_result = client_connect(client);
    if (conn_result != CLIENT_SUCCESS)
    {
        return conn_result;
    }

    char response[get_client_buffer_size()];
    client_result_t result = client_send_command(client, "PING\r\n", response, sizeof(response));

    pthread_mutex_lock(&client->lock);
    client->stats.operations_total++;
    if (result != CLIENT_SUCCESS)
    {
        client->stats.operations_failed++;
    }
    pthread_mutex_unlock(&client->lock);

    return result;
}

// ==================== Client Information API Implementation ====================

client_status_t client_get_status(const client_instance_t *client)
{
    if (client == NULL)
    {
        return CLIENT_STATUS_DISCONNECTED;
    }
    return client->status;
}

bool client_get_stats(const client_instance_t *client, client_stats_t *stats)
{
    if (client == NULL || stats == NULL)
    {
        return false;
    }

    pthread_mutex_lock((pthread_mutex_t *)&client->lock); // Cast away const for internal sync
    *stats = client->stats;

    // Update connection time if currently connected
    if (client->status == CLIENT_STATUS_CONNECTED && client->connect_time > 0)
    {
        stats->connection_time_seconds += difftime(time(NULL), client->connect_time);
    }

    pthread_mutex_unlock((pthread_mutex_t *)&client->lock);
    return true;
}

const client_config_t *client_get_config(const client_instance_t *client)
{
    if (client == NULL)
    {
        return NULL;
    }
    return &client->config;
}

const char *client_get_last_error(const client_instance_t *client)
{
    if (client == NULL || client->last_error[0] == '\0')
    {
        return "No error";
    }
    return client->last_error;
}

bool client_is_connected(const client_instance_t *client)
{
    if (client == NULL)
    {
        return false;
    }
    return client->status == CLIENT_STATUS_CONNECTED;
}

// ==================== Utility Functions Implementation ====================

client_config_t client_config_default(void)
{
    client_config_t config;
    config.host = get_client_default_host();
    config.port = get_client_default_port();
    config.timeout_ms = get_client_default_timeout();
    config.max_retries = get_client_max_retries();
    config.auto_reconnect = get_client_auto_reconnect();
    return config;
}

bool client_config_validate(const client_config_t *config, char *error_buffer, size_t error_size)
{
    if (config == NULL)
    {
        snprintf(error_buffer, error_size, "Configuration is NULL");
        return false;
    }

    if (config->host == NULL || strlen(config->host) == 0)
    {
        snprintf(error_buffer, error_size, "Host cannot be empty");
        return false;
    }

    if (config->port < 1 || config->port > 65535)
    {
        snprintf(error_buffer, error_size, "Invalid port: %u (must be 1-65535)", config->port);
        return false;
    }

    if (config->timeout_ms == 0)
    {
        snprintf(error_buffer, error_size, "Timeout cannot be zero");
        return false;
    }

    return true;
}

const char *client_result_to_string(client_result_t result)
{
    switch (result)
    {
    case CLIENT_SUCCESS:
        return "Success";
    case CLIENT_ERROR_CONNECTION:
        return "Connection error";
    case CLIENT_ERROR_TIMEOUT:
        return "Timeout error";
    case CLIENT_ERROR_PROTOCOL:
        return "Protocol error";
    case CLIENT_ERROR_SERVER:
        return "Server error";
    case CLIENT_ERROR_MEMORY:
        return "Memory error";
    default:
        return "Unknown error";
    }
}
