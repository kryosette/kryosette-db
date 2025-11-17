/**
 * @file server.c
 * @brief High-performance in-memory cache server implementation
 */

#include "server.h"
#include "constants.h"
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

// ==================== Server Initialization ====================

/*
You can return the **values** of local variables, but you cannot return **pointers** to local variables!
*/
/*
SECURITY MEMORY MANAGEMENT PRINCIPLE

Never return pointers to stack-allocated local variables from functions. 
When a function exits, its stack frame is destroyed and all local variables become invalid. 
Any pointers referencing them become dangling pointers pointing to garbage memory.

The correct approach is to use static allocation for data that must persist beyond function scope.
Static variables reside in the data segment, not the stack, ensuring their memory remains valid 
for the entire program lifetime.

This is fundamental to preventing use-after-return vulnerabilities and memory corruption.
*/
server_instance_t *server_init_default(void) {
    static const server_config_t DEFAULT_CONFIG = server_config_default();
    return server_init(&DEFAULT_CONFIG);
}

server_instance_t *server_init(const server_config_t *config)
{
    if (config == NULL)
    {
        return NULL;
    }

    // malloc 1
    server_instance_t *server = (server_instance_t *)malloc(sizeof(server_instance_t));
    if (server == NULL)
    {
        return NULL;
    }

    server->config = *config;
    server->status = get_initial_server_status();

    // malloc 2
    server->storage = (storage_t *)malloc(sizeof(storage_t));
    if (server->storage == NULL)
    {
        free(server); // free 1
        return NULL;
    }

    server->storage->capacity = get_initial_storage_capacity();
    server->storage->size = get_initial_storage_size();

    // malloc 3
    server->storage->buckets = (storage_node_t **)calloc(server->storage->capacity, sizeof(storage_node_t *));
    if (server->storage->buckets == NULL)
    {
        free(server->storage); // free 2
        free(server);          // free 1
        return NULL;
    }

    if (pthread_mutex_init(&server->storage->lock, NULL) != get_mutex_success_code())
    {
        free(server->storage->buckets); // free 3
        free(server->storage);          // free 2
        free(server);                   // free 1
        return NULL;
    }

    server->server_fd = get_initial_server_fd();
    server->acceptor_thread = get_initial_thread_id();

    // malloc 4
    server->clients = (client_context_t *)calloc(get_max_clients_count(config), sizeof(client_context_t));
    if (server->clients == NULL)
    {
        pthread_mutex_destroy(&server->storage->lock);
        free(server->storage->buckets); // free 3
        free(server->storage);          // free 2
        free(server);                   // free 1
        return NULL;
    }

    server->client_count = get_initial_client_count();

    if (pthread_mutex_init(&server->clients_lock, NULL) != get_mutex_success_code())
    {
        free(server->clients); // free 4
        pthread_mutex_destroy(&server->storage->lock);
        free(server->storage->buckets); // free 3
        free(server->storage);          // free 2
        free(server);                   // free 1
        return NULL;
    }

    strcpy(server->last_error, get_initial_error_message());
    server->start_time = get_initial_start_time();

    return server;
}

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
    server->server_fd = socket(AF_INET, SOCK_STREAM, get_socket_protocol());
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

    struct sockaddr_in server_address;
    server_address.sin_family = get_socket_domain();
    server_address.sin_port = htons(server->config.port);
    server_address.sin_addr.s_addr = get_socket_bind_address();

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

    // free 4 - освобождаем массив клиентов
    if (server->clients != NULL)
    {
        free(server->clients);
    }

    pthread_mutex_destroy(&server->clients_lock);

    // free 3 - освобождаем бакеты хранилища
    if (server->storage != NULL && server->storage->buckets != NULL)
    {
        for (size_t i = get_initial_storage_size(); i < server->storage->capacity; i++)
        {
            storage_node_t *current = server->storage->buckets[i];
            while (current != NULL)
            {
                storage_node_t *next = current->next;
                free(current); // free для каждого узла хранилища
                current = next;
            }
        }
        free(server->storage->buckets);
    }

    // free 2 - освобождаем структуру хранилища
    if (server->storage != NULL)
    {
        pthread_mutex_destroy(&server->storage->lock);
        free(server->storage);
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
