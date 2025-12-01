#include "/Users/dimaeremin/kryosette-db/kryocache/src/core/server/commands/include/commands.h"
#include "/Users/dimaeremin/kryosette-db/kryocache/src/core/server/commands/include/constants.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdbool.h>

unsigned int hash(const char *key);
bool storage_set(const char *key, const char *value);
const char *storage_get(const char *key);

void handle_client_connection(int client_fd)
{
    char buffer[1024];
    /*
    ssize_t
        Used for a count of bytes or an error indication.  It is a
        signed integer type capable of storing values at least in
        the range [-1, SSIZE_MAX].
    */
    ssize_t bytes_read;
    
    // -1 for /0
    bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        printf("Received command: %s", buffer);
        
        char *command = buffer;
        if (bytes_read > 0) {
            if (command[bytes_read - 1] == '\n') {
                command[bytes_read - 1] = '\0';
                bytes_read--;  
            }
    
            if (bytes_read > 0 && command[bytes_read - 1] == '\r') {
                command[bytes_read - 1] = '\0';
            }
        }

        /*
        #include <sys/socket.h>

        The system calls send(), sendto(), and sendmsg() are used to
        transmit a message to another socket.

        The send() call may be used only when the socket is in a connected
        state (so that the intended recipient is known).  The only
        difference between send() and write(2) is the presence of flags.
        With a zero flags argument, send() is equivalent to write(2).
        Also, the following call

        send(sockfd, buf, size, flags);

        is equivalent to

        sendto(sockfd, buf, size, flags, NULL, 0);

        The argument sockfd is the file descriptor of the sending socket.


        ssize_t send(size_t size;
                      int sockfd, const void buf[size], size_t size, int flags);
        ssize_t sendto(size_t size;
                      int sockfd, const void buf[size], size_t size, int flags,
                      const struct sockaddr *dest_addr, socklen_t addrlen);
        ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags);

        */
        if (strncmp(command, "PING", 5) == 0) {
            const char *response = "PONG\r\n";
            send(client_fd, response, strlen(response), 0);
            printf("Sent PONG response\n");
        }
        else if (strncmp(command, "FLUSH", 5) == 0) {
            for (int i = 0; i < 1000; i++) {
                storage_node_db_t *node = g_storage.buckets[i];
                while (node) {
                    storage_node_db_t *next = node->next;
                    free(node);
                    node = next;
                }
                g_storage.buckets[i] = NULL;
            }
            g_storage.size = 0;
            const char *response = "OK\r\n";
            send(client_fd, response, strlen(response), 0);
            printf("Sent FLUSH OK response\n");
        }
        /*
        strncmp — compare part of two strings

        #include <string.h>

        The core difference boils down to one of safety: strncmp allows you to limit the comparison, 
        preventing potential buffer overruns and undefined behavior, while strcmp does not.

        int strcmp(const char *s1, const char *s2);
        int strncmp(const char *s1, const char *s2, size_t n);
        */
        else if (strncmp(command, "SET ", 4) == 0) {
            char *key = command + 4;
            char *value = strchr(key, ' ');
            if (value) {
                *value = '\0';
                value++;
                if (storage_set(key, value)) {
                    const char *response = "OK\r\n";
                    send(client_fd, response, strlen(response), 0);
                    printf("Sent SET OK response for key: %s\n", key);
                } else {
                    const char *response = "ERROR Memory full\r\n";
                    send(client_fd, response, strlen(response), 0);
                    printf("Sent ERROR for SET\n");
                }
            } else {
                const char *response = "ERROR Invalid SET format\r\n";
                send(client_fd, response, strlen(response), 0);
                printf("Sent ERROR for invalid SET\n");
            }
        }
        else if (strncmp(command, "GET ", 4) == 0) {
            char *key = command + 4;
            const char *value = storage_get(key);
            if (value) {
                char response[512];
                snprintf(response, sizeof(response), "VALUE %s\r\n", value);
                send(client_fd, response, strlen(response), 0);
                printf("Sent GET response for key: %s -> %s\n", key, value);
            } else {
                const char *response = "NOT_FOUND\r\n";
                send(client_fd, response, strlen(response), 0);
                printf("Sent NOT_FOUND for key: %s\n", key);
            }
        }
        else if (strncmp(command, "DELETE ", 7) == 0) {
            char *key = command + 7;
            const char *response = "OK\r\n";
            send(client_fd, response, strlen(response), 0);
            printf("Sent DELETE OK response\n");
        }
        else if (strncmp(command, "EXISTS ", 7) == 0) {
            char *key = command + 7;
            const char *value = storage_get(key);
            if (value) {
                const char *response = "1\r\n"; // 1 = exists
                send(client_fd, response, strlen(response), 0);
                printf("Sent EXISTS 1 for key: %s\n", key);
            } else {
                const char *response = "0\r\n"; // 0 = not exists
                send(client_fd, response, strlen(response), 0);
                printf("Sent EXISTS 0 for key: %s\n", key);
            }
        }
        else if (strncmp(command, "STATS", 6) == 0) {
            char response[128];
            snprintf(response, sizeof(response), "KEYS: %zu\r\n", g_storage.size);
            send(client_fd, response, strlen(response), 0);
            printf("Sent STATS response: %s", response);
        }
        else {
            const char *response = "ERROR Unknown command\r\n";
            send(client_fd, response, strlen(response), 0);
            printf("Sent ERROR response for unknown command\n");
        }
    } else {
        printf("Client disconnected or error reading command\n");
    }
    
    close(client_fd);
}

bool storage_set(const char *key, const char *value) {
    unsigned int index = hash(key); 
    /*
    In computer science, "buckets" are containers for holding related data.
    */
    storage_node_db_t *node = g_storage.buckets[index]; // until 64

    while (node) {
        if (strcmp(node->key, key) == 0) {
            strncpy(node->value, value, sizeof(node->value) - 1);
            node->value[sizeof(node->value) - 1] = '\0';
            return true;
        }
        node = node->next;
    }

    storage_node_db_t *new_node = calloc(sizeof(storage_node_db_t));
    if (!new_node) return false;
    /*
    The strdup() function returns a pointer to a new string which is a
       duplicate of the string s.  Memory for the new string is obtained
       with malloc(3), and can be freed with free(3).
    */
    strncpy(new_node->key, key, sizeof(new_node->key) - 1);
    new_node->key[sizeof(new_node->key) - 1] = '\0';
    
    strncpy(new_node->value, value, sizeof(new_node->value) - 1);
    new_node->value[sizeof(new_node->value) - 1] = '\0';
    
    new_node->next = g_storage.buckets[index];
    g_storage.buckets[index] = new_node;
    g_storage.size++;

    return true;
}

const char *storage_get(const char *key) {
    unsigned int index = hash(key);  
    storage_node_db_t *node = g_storage.buckets[index];
    
    while (node) {
        if (strncmp(node->key, key) == 0) {
            return node->value;
        }
        node = node->next;
    }

    return NULL;   
}