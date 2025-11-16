/**
 * @file server.c
 * @brief High-performance in-memory cache server implementation
 */

#include "/mnt/c/Users/dmako/kryosette/kryosette-db/kryocache/src/core/server/include/server.h"
#include "/mnt/c/Users/dmako/kryosette/kryosette-db/kryocache/src/core/server/include/constants.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <strings.h> // for strcasecmp

server_instance_t *server_init(const server_config_t *config)
{
    if (config == NULL)
    {
        return NULL;
    }

    // free 1
    server_instance_t *server = (server_instance_t *)malloc(sizeof(server_instance_t));
    if (server == NULL)
    {
        return NULL;
    }

    server->config = *config;
    server->status = get_initial_server_status();

    // free 2
    server->storage = (storage_t *)malloc(sizeof(storage_t));
    if (server->storage == NULL)
    {
        free(server); // free 1
        return NULL;
    }

    server->storage->capacity = get_initial_storage_capacity();
    server->storage->size = get_initial_storage_size();

    // free 3
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

    // free 4
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