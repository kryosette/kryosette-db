/**
 * @file main.c
 * @brief Server main entry point
 */

#include "/mnt/c/Users/dmako/kryosette/kryosette-db/kryocache/src/core/server/include/server.h"
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

static server_instance_t *g_server = NULL;

void signal_handler(int sig)
{
    printf("\nReceived signal %d, shutting down...\n", sig);
    if (g_server)
    {
        server_stop(g_server, 5000);
    }
}

void on_client_connect(int client_id, const char *client_ip, void *user_data)
{
    printf("Client %d connected from %s\n", client_id, client_ip);
}

void on_client_disconnect(int client_id, void *user_data)
{
    printf("Client %d disconnected\n", client_id);
}

void on_command(int client_id, const char *command, void *user_data)
{
    printf("Client %d executed: %s\n", client_id, command);
}

int main()
{
    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    printf("Starting In-Memory Cache Server...\n");

    // Create server with default configuration
    g_server = server_init_default();
    if (!g_server)
    {
        fprintf(stderr, "Failed to initialize server\n");
        return 1;
    }

    // Set up callbacks
    server_set_connect_callback(g_server, on_client_connect, NULL);
    server_set_disconnect_callback(g_server, on_client_disconnect, NULL);
    server_set_command_callback(g_server, on_command, NULL);

    // Start the server
    if (!server_start(g_server))
    {
        fprintf(stderr, "Failed to start server: %s\n", server_get_last_error(g_server));
        server_destroy(g_server);
        return 1;
    }

    printf("Server is running. Press Ctrl+C to stop.\n");

    // Main loop (simple sleep)
    while (server_get_status(g_server) == SERVER_STATUS_RUNNING)
    {
        sleep(1);

        // Print stats every 10 seconds
        static time_t last_stats = 0;
        time_t now = time(NULL);
        if (now - last_stats >= 10)
        {
            server_stats_t stats;
            if (server_get_stats(g_server, &stats))
            {
                printf("Stats: %zu keys, %u clients, %.0fs uptime\n",
                       stats.keys_stored, stats.connected_clients, stats.uptime_seconds);
            }
            last_stats = now;
        }
    }

    // Cleanup
    server_destroy(g_server);
    printf("Server stopped.\n");

    return 0;
}