/**
 * @file server.c
 * @brief High-performance in-memory cache server implementation
 */
#include "/Users/dimaeremin/kryosette-db/kryocache/src/core/server/include/server.h"
#include "/Users/dimaeremin/kryosette-db/kryocache/src/core/server/include/constants.h"
#include "/Users/dimaeremin/kryosette-db/third-party/smemset/include/smemset.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <strings.h>
#include <arpa/inet.h>    
#include <errno.h>      

// ==================== Internal Thread Functions ====================

static void *server_acceptor_thread(void *arg)
{
    server_instance_t *server = (server_instance_t *)arg;

    printf("Acceptor thread started on port %d\n", server->config.port);

    while (server->status == SERVER_STATUS_RUNNING)
    {
        /*
            struct sockaddr_in6 {
               sa_family_t     sin6_family;    AF_INET6 
               in_port_t       sin6_port;      port number 
               uint32_t        sin6_flowinfo;  IPv6 flow information 
               struct in6_addr sin6_addr;     IPv6 address 
               uint32_t        sin6_scope_id;  Scope ID (new in Linux 2.4) 
            };

            struct in6_addr {
               unsigned char   s6_addr[16];   IPv6 address 
            };
        */
        struct sockaddr_in6 client_addr;
        socklen_t client_len = sizeof(client_addr);

        /*
        #include <sys/socket.h>

        int accept(int sockfd, struct sockaddr *_Nullable restrict addr,
                  socklen_t *_Nullable restrict addrlen);

        #define _GNU_SOURCE              See feature_test_macros(7) 
        #include <sys/socket.h>

        int accept4(int sockfd, struct sockaddr *_Nullable restrict addr,
                  socklen_t *_Nullable restrict addrlen, int flags);
        */
        int client_fd = accept(server->server_fd, (struct sockaddr*)&client_addr, &client_addr);

        if (client_fd >= 0) {
            char client_ip[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, &client_addr.sin6_addr, client_ip, sizeof(client_ip));
            printf("New client connected from %s:%d\n", client_ip, ntohs(client_addr.sin6_port));
            
            handle_client_connection(client_fd);
        } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("accept failed");
            break;
        }

        usleep(100000);
    }

    printf("Acceptor thread stopped\n");
    return NULL;
}


// ==================== Server Initialization ====================

/*
You can return the **values** of local variables, but you cannot return **pointers** to local variables!

SECURITY MEMORY MANAGEMENT PRINCIPLE

Never return pointers to stack-allocated local variables from functions.
When a function exits, its stack frame is destroyed and all local variables become invalid.
Any pointers referencing them become dangling pointers pointing to garbage memory.

The correct approach is to use static allocation for data that must persist beyond function scope.
Static variables reside in the data segment, not the stack, ensuring their memory remains valid
for the entire program lifetime.

This is fundamental to preventing use-after-return vulnerabilities and memory corruption.

ERRORS:
✖  static const server_config_t DEFAULT_CONFIG = server_config_default();
⭡⭡⭡⭡ Error: initializer element is not constant ⭡⭡⭡⭡

Reason: In C, the compiler requires compile-time constant expressions to initialize static const variables.
The server_config_default() function is executed in runtime, so the compiler cannot calculate its value at the compilation stage.
*/
server_instance_t *server_init_default(void)
{
    static server_config_t DEFAULT_CONFIG;
    static int initialized = 0;

    if (!initialized)
    {
        DEFAULT_CONFIG.port = get_server_default_port();
        DEFAULT_CONFIG.max_clients = get_server_max_clients();
        DEFAULT_CONFIG.max_memory = get_default_max_memory();
        DEFAULT_CONFIG.mode = get_default_server_mode();
        DEFAULT_CONFIG.bind_address = get_default_bind_address();
        DEFAULT_CONFIG.data_directory = get_default_data_directory();
        DEFAULT_CONFIG.persistence_enabled = get_default_persistence_enabled();
        DEFAULT_CONFIG.persistence_interval = get_default_persistence_interval();
        initialized = 1;
    }

    return server_init(&DEFAULT_CONFIG);
}

/*
MEMORY INITIALIZATION SAFETY PRINCIPLE

Always initialize allocated memory to a known state before use.
Uninitialized memory from malloc() contains garbage data which can lead to
undefined behavior, security vulnerabilities, and difficult-to-debug issues.

The calloc() function automatically zeros memory, providing safe initialization.
Alternatively, memset() can be used after malloc() to achieve the same effect.

This prevents:
- Information leaks (uninitialized memory containing sensitive data)
- Random pointer values causing crashes
- Non-deterministic behavior from uninitialized variables
*/
server_instance_t *server_init(const server_config_t *config)
{
    if (config == NULL)
    {
        return NULL;
    }

    /*
    MEMORY SAFETY: ZERO-INITIALIZATION
    Using calloc instead of malloc ensures all fields start as zero/NULL
    This prevents uninitialized pointer issues and garbage data
    */
    server_instance_t *server = (server_instance_t *)calloc(1, sizeof(server_instance_t));
    if (server == NULL)
    {
        return NULL;
    }

    server->config = *config;
    server->status = get_initial_server_status();

    /*
    MEMORY SAFETY: CONSISTENT INITIALIZATION
    Continue using calloc for all allocations to maintain zero-initialized state
    */
    server->storage = (storage_t *)calloc(1, sizeof(storage_t));
    if (server->storage == NULL)
    {
        free(server);
        return NULL;
    }

    /*
    THREAD SAFETY PRINCIPLE: PROTECTION BEFORE DATA

    Always initialize synchronization primitives (mutexes, locks) BEFORE making data accessible.
    When multiple threads can access shared data, the protection mechanism must be fully
    established before any thread can potentially access that data.

    Initializing data first and then creating the mutex creates a race condition window where
    data is accessible but unprotected. This violates fundamental thread safety requirements.
    */
    if (pthread_mutex_init(&server->storage->lock, NULL) != 0)
    {
        free(server->storage);
        free(server);
        return NULL;
    }

    /*
    DATA INITIALIZATION UNDER PROTECTION
    Now that mutex is established, we can safely initialize the protected data
    The mutex ensures thread-safe access from this point forward
    */
    server->storage->capacity = get_initial_storage_capacity();
    server->storage->size = get_initial_storage_size();

    server->storage->buckets = (storage_node_t **)calloc(server->storage->capacity, sizeof(storage_node_t *));
    if (server->storage->buckets == NULL)
    {
        pthread_mutex_destroy(&server->storage->lock);
        free(server->storage);
        free(server);
        return NULL;
    }

    server->server_fd = get_initial_server_fd();
    server->acceptor_thread = get_initial_thread_id();

    /*
    MEMORY ALLOCATION SAFETY: SIZE VALIDATION
    Validate dynamic size calculations to prevent excessive memory allocation
    */
    size_t max_clients = get_max_clients_count(config);
    size_t MAX_SAFE_CLIENT_COUNT = get_max_safe_client_count();
    size_t DEFAULT_CLIENT_COUNT = get_default_client_count();
    if (max_clients == 0 || max_clients > MAX_SAFE_CLIENT_COUNT)
    {
        max_clients = DEFAULT_CLIENT_COUNT;
    }

    server->actual_max_clients = max_clients;
    server->clients = (client_context_t *)calloc(max_clients, sizeof(client_context_t));
    if (server->clients == NULL)
    {
        pthread_mutex_destroy(&server->storage->lock);
        free(server->storage->buckets);
        free(server->storage);
        free(server);
        return NULL;
    }

    server->client_count = get_initial_client_count();

    /*
    THREAD SAFETY: CLIENT DATA PROTECTION
    Initialize client mutex before any client data becomes accessible
    This ensures client operations are thread-safe from the beginning
    */
    if (pthread_mutex_init(&server->clients_lock, NULL) != 0)
    {
        free(server->clients);
        pthread_mutex_destroy(&server->storage->lock);
        free(server->storage->buckets);
        free(server->storage);
        free(server);
        return NULL;
    }

    /*
    BUFFER SAFETY PRINCIPLE: BOUNDED STRING COPY

    Never use unbounded string functions like strcpy() which can lead to
    buffer overflows and memory corruption. Always use length-limited
    alternatives and explicitly null-terminate the destination buffer.

    Buffer overflows can lead to:
    - Stack/heap corruption
    - Arbitrary code execution
    - Information disclosure
    - Program instability
    */
    const char *error_msg = get_initial_error_message();
    if (error_msg != NULL)
    {
        strncpy(server->last_error, error_msg, sizeof(server->last_error) - 1);
        server->last_error[sizeof(server->last_error) - 1] = '\0';
    }
    else
    {
        server->last_error[0] = '\0';
    }

    server->start_time = get_initial_start_time();

    /*
    RESOURCE TRACKING PRINCIPLE

    In production code, consider tracking all allocated resources for
    comprehensive cleanup and leak detection. This becomes especially
    important in long-running server applications where resource leaks
    accumulate over time.

    Simple approach: Set a cleanup flag or use RAII patterns
    Advanced approach: Resource tracking with rollback capabilities
    */
    return server;
}

/*
CLEANUP SAFETY PRINCIPLE

Always provide symmetric cleanup functions that mirror initialization.
Every allocated resource must have a corresponding deallocation path.
Cleanup functions should be idempotent (safe to call multiple times)
and handle partial initialization states gracefully.

Recommended cleanup function:

void server_destroy(server_instance_t *server) {
    if (!server) return;

    if (server->clients) {
        pthread_mutex_destroy(&server->clients_lock);
        free(server->clients);
    }

    if (server->storage) {
        if (server->storage->buckets) {
            pthread_mutex_destroy(&server->storage->lock);
            free(server->storage->buckets);
        }
        free(server->storage);
    }

    free(server);
}
*/

// ==================== Server Control Functions ====================

bool server_start(server_instance_t *server)
{
    if (server == NULL)
    {
        return false;
    }

    if (server->status == SERVER_STATUS_RUNNING)
    {
        return true;
    }

    server->status = SERVER_STATUS_STARTING;

    // malloc 5 - создание сокета
    server->server_fd = socket(AF_INET6, SOCK_STREAM, get_socket_protocol());
    if (server->server_fd < get_socket_success_code())
    {
        server->status = SERVER_STATUS_ERROR;
        strcpy(server->last_error, get_socket_creation_error_message());
        return false;
    }

    int socket_option = get_socket_reuseaddr_option();
    if (setsockopt(server->server_fd, get_socket_level(), SO_REUSEADDR,
                   &socket_option, sizeof(socket_option)) < get_socket_success_code())
    {
        server->status = SERVER_STATUS_ERROR;
        strcpy(server->last_error, get_socket_option_error_message());
        return false;
    }

    struct sockaddr_in6 server_address;
    server_address.sin6_family = get_socket_domain();
    server_address.sin6_port = htons(server->config.port);
    server_address.sin6_addr = in6addr_any;

    if (bind(server->server_fd, (struct sockaddr *)&server_address,
             sizeof(server_address)) < get_socket_success_code())
    {
        server->status = SERVER_STATUS_ERROR;
        strcpy(server->last_error, get_socket_bind_error_message());
        return false;
    }

    if (listen(server->server_fd, get_socket_backlog()) < get_socket_success_code())
    {
        server->status = SERVER_STATUS_ERROR;
        strcpy(server->last_error, get_socket_listen_error_message());
        return false;
    }

    server->status = SERVER_STATUS_RUNNING;
    server->start_time = time(NULL);

    if (pthread_create(&server->acceptor_thread, NULL,
                       server_acceptor_thread, server) != get_thread_success_code())
    {
        server->status = SERVER_STATUS_ERROR;
        strcpy(server->last_error, get_thread_creation_error_message());
        return false;
    }

    return true;
}

bool server_stop(server_instance_t *server, uint32_t timeout_ms)
{
    if (server == NULL)
    {
        return false;
    }

    if (server->status != SERVER_STATUS_RUNNING)
    {
        return true;
    }

    server->status = SERVER_STATUS_SHUTTING_DOWN;

    // Закрываем основной сокет сервера
    if (server->server_fd >= get_initial_server_fd())
    {
        shutdown(server->server_fd, get_socket_shutdown_mode());
    }

    time_t start_wait_time = time(NULL);
    while (server->client_count > get_initial_client_count() &&
           time(NULL) - start_wait_time < timeout_ms / get_milliseconds_per_second())
    {
        sleep(get_polling_interval_seconds());
    }

    if (server->client_count > get_initial_client_count())
    {
        server_force_shutdown(server);
        return false;
    }

    server->status = SERVER_STATUS_STOPPED;
    return true;
}

void server_force_shutdown(server_instance_t *server)
{
    if (server == NULL)
    {
        return;
    }

    server->status = SERVER_STATUS_STOPPED;

    if (server->server_fd >= get_initial_server_fd())
    {
        shutdown(server->server_fd, get_socket_shutdown_mode());
    }
}

void server_destroy(server_instance_t *server)
{
    if (server == NULL)
    {
        return;
    }

    if (server->status == SERVER_STATUS_RUNNING)
    {
        server_force_shutdown(server);
    }

    if (server->clients == NULL && server->storage == NULL)
    {
        printf("DEBUG: server already destroyed, skipping\n");
        free(server);
        return;
    }

    // free 4 - освобождаем массив клиентов
    if (server->clients != NULL)
    {
        uint32_t clients_to_clean = server->client_count;
        if (clients_to_clean > server->actual_max_clients)
        {
            clients_to_clean = server->actual_max_clients;
        }

        for (size_t i = 0; i < clients_to_clean; i++)
        {
            if (server->clients[i].fd >= 0)
            {
                // pay attention to safety 💥
                close(server->clients[i].fd);
                server->clients[i].fd = -1;
            }

            server->clients[i].connected = false;

            /* void *smemset(void *s, int c, size_t n);
            s: A pointer to the starting address of the memory block to be filled. This is a void* pointer, allowing it to accept pointers to any data type.
            c: The value to be set. This is passed as an int, but memset uses the unsigned char representation of this value to fill the memory.
            n: The number of bytes to fill, starting from the address pointed to by s. size_t is an unsigned integral type.

            smemset(&server->clients[i].addr, 0, sizeof(struct sockaddr_in));
            sizeof: In C (unlike C++), when you declare a structure, you need to use the struct keyword before the name.
            but I'd rather do a typedef.
            */
            smemset(&server->clients[i].addr, 0, sizeof(sockaddr_in_t));
        }

        free(server->clients);
        server->clients = NULL;
    }

    pthread_mutex_destroy(&server->clients_lock);

    // free 3 - освобождаем бакеты хранилища
    if (server->storage != NULL)
    {
        // Освобождаем все узлы в бакетах
        if (server->storage->buckets != NULL)
        {
            for (size_t i = 0; i < server->storage->capacity; i++) // Начинаем с 0!
            {
                storage_node_t *current = server->storage->buckets[i];
                while (current != NULL)
                {
                    storage_node_t *next = current->next;
                    free(current);
                    current = next;
                }
                server->storage->buckets[i] = NULL;
            }
            free(server->storage->buckets);
            server->storage->buckets = NULL;
        }

        pthread_mutex_destroy(&server->storage->lock);
        free(server->storage);
        server->storage = NULL; // Важно: обнуляем указатель
    }

    // free 1 - освобождаем основную структуру сервера
    free(server);
}

// ==================== Server Information Functions ====================

server_status_t server_get_status(const server_instance_t *server)
{
    if (server == NULL)
    {
        return get_server_status_stopped();
    }
    return server->status;
}

bool server_get_stats(const server_instance_t *server, server_stats_t *stats)
{
    if (server == NULL || stats == NULL)
    {
        return false;
    }

    stats->connections_total = get_initial_connection_count();
    stats->commands_processed = get_initial_command_count();
    stats->keys_stored = server->storage->size;
    stats->memory_used = get_initial_memory_usage();
    stats->connected_clients = server->client_count;
    stats->uptime_seconds = get_server_uptime_seconds(server);

    return true;
}

const server_config_t *server_get_config(const server_instance_t *server)
{
    if (server == NULL)
    {
        return NULL;
    }
    return &server->config;
}

const char *server_get_last_error(const server_instance_t *server)
{
    if (server == NULL || server->last_error[get_string_start_index()] == get_string_terminator())
    {
        return get_empty_string();
    }
    return server->last_error;
}

// ==================== Configuration Functions ====================

server_config_t server_config_default(void)
{
    server_config_t config;
    config.port = get_server_default_port();
    config.max_clients = get_server_max_clients();
    config.max_memory = get_default_max_memory();
    config.mode = get_default_server_mode();
    config.bind_address = get_default_bind_address();
    config.data_directory = get_default_data_directory();
    config.persistence_enabled = get_default_persistence_enabled();
    config.persistence_interval = get_default_persistence_interval();
    return config;
}

bool server_config_validate(const server_config_t *config, char *error_buffer, size_t error_size)
{
    if (config == NULL)
    {
        snprintf(error_buffer, error_size, get_null_config_error_message());
        return false;
    }

    // MINIMUM_PORT_NUMBER = 1024 || MAXIMUM_PORT_NUMBER = 65535
    if (config->port < get_minimum_port_number() || config->port > get_maximum_port_number())
    {
        snprintf(error_buffer, error_size, get_invalid_port_error_message(), config->port);
        return false;
    }

    if (config->max_clients < get_minimum_client_count())
    {
        snprintf(error_buffer, error_size, get_invalid_client_count_error_message());
        return false;
    }

    return true;
}

bool server_config_load(const char *filename, server_config_t *config, char *error_buffer, size_t error_size)
{
    if (filename == NULL || config == NULL)
    {
        snprintf(error_buffer, error_size, get_null_parameter_error_message());
        return false;
    }

    *config = server_config_default();
    return true;
}

bool server_config_save(const char *filename, const server_config_t *config)
{
    if (filename == NULL || config == NULL)
    {
        return false;
    }
    return true;
}

// ==================== Advanced Features ====================

bool server_save_data(server_instance_t *server)
{
    if (server == NULL)
    {
        return false;
    }
    return true;
}

bool server_load_data(server_instance_t *server)
{
    if (server == NULL)
    {
        return false;
    }
    return true;
}

bool server_flush_data(server_instance_t *server)
{
    if (server == NULL || server->storage == NULL)
    {
        return false;
    }

    pthread_mutex_lock(&server->storage->lock);

    for (size_t i = get_initial_storage_size(); i < server->storage->capacity; i++)
    {
        storage_node_t *current = server->storage->buckets[i];
        while (current != NULL)
        {
            storage_node_t *next = current->next;
            free(current); // free для каждого узла при flush
            current = next;
        }
        server->storage->buckets[i] = NULL;
    }

    server->storage->size = get_initial_storage_size();
    pthread_mutex_unlock(&server->storage->lock);

    return true;
}

const char *server_get_version(void)
{
    return get_server_version_string();
}

const char *server_get_build_info(void)
{
    return get_server_build_info_string();
}
