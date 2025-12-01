#include "/Users/dimaeremin/kryosette-db/kryocache/src/core/server/commands/include/commands.h"
#include "/Users/dimaeremin/kryosette-db/kryocache/src/core/server/commands/include/constants.h"

static void handle_client_connection(int client_fd)
{
    char buffer[1024];
    /*
    ssize_t
        Used for a count of bytes or an error indication.  It is a
        signed integer type capable of storing values at least in
        the range [-1, SSIZE_MAX].
    */
    ssize_t bytes_read;
    
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
            const char *response = "OK\r\n";
            send(client_fd, response, strlen(response), 0);
            printf("Sent SET OK response\n");
        }
        else if (strncmp(command, "GET ", 4) == 0) {
            const char *response = "VALUE example_value\r\n";
            send(client_fd, response, strlen(response), 0);
            printf("Sent GET response\n");
        }
        else if (strncmp(command, "DELETE ", 7) == 0) {
            const char *response = "OK\r\n";
            send(client_fd, response, strlen(response), 0);
            printf("Sent DELETE OK response\n");
        }
        else if (strncmp(command, "EXISTS ", 7) == 0) {
            const char *response = "1\r\n"; // 1 = exists, 0 = not exists
            send(client_fd, response, strlen(response), 0);
            printf("Sent EXISTS response\n");
        }
        else if (strncmp(command, "STATS", 6) == 0) {
            const char *response = "STATS: 0 keys, 0 clients, 0s uptime\r\n";
            send(client_fd, response, strlen(response), 0);
            printf("Sent STATS response\n");
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
