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

#include <sys/time.h> // для struct timeval
#include <fcntl.h>    // для fcntl, F_GETFL, F_SETFL, O_NONBLOCK
#include <poll.h>     // для poll, struct pollfd, POLLOUT, POLLERR и т.д.
#include <sys/select.h>

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

    /*
    struct in6_addr {
        unsigned char   s6_addr[16];  IPv6 address
    };
    */
    struct in6_addr ipv6_addr;
    if (inet_pton(AF_INET6, client->config.host, &ipv6_addr) != 1)
    {
        snprintf(client->last_error, sizeof(client->last_error),
                 "Invalid IPv6 address: %s", client->config.host);
        return CLIENT_ERROR_PROTOCOL;
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

    but! I only use ipv6 without dns

    struct sockaddr_in6 {
        sa_family_t     sin6_family;   /* AF_INET6
        in_port_t sin6_port;       /* port number
        uint32_t sin6_flowinfo;    /* IPv6 flow information
        struct in6_addr sin6_addr; /* IPv6 address
        uint32_t sin6_scope_id;    /* Scope ID (new in Linux 2.4)
    };
    */
    struct sockaddr_in6 server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_port = htons(client->config.port);
    memcpy(&server_addr.sin6_addr, &ipv6_addr, sizeof(ipv6_addr));
    // Resolve hostname to IP address
    /*
    int inet_pton(int af, const char *restrict src, void *restrict dst);

    *a function for converting a string with an IP address to a binary format
    This function converts the character string src into a network
       address structure in the af address family, then copies the
       network address structure to dst.  The af argument must be either
       AF_INET or AF_INET6.  dst is written in network byte order.
    */
    // if (inet_pton(AF_INET6, client->config.host, &server_addr->sin6_addr) <= 0)
    // {
    //     // gethostbyname - deprecated! don't use this!
    //     // struct hostent *he = gethostbyname(client->config.host);
    //     if (he == NULL)
    //     {
    //         snprintf(client->last_error, sizeof(client->last_error),
    //                  "Host resolution failed for: %s", client->config.host);
    //         close(client->sockfd);
    //         client->sockfd = -1;
    //         return CLIENT_ERROR_CONNECTION;
    //     }
    //     memcpy(&client->server_addr.sin_addr, he->h_addr_list[0], he->h_length);
    // }

    memcpy(&client->server_addr, &server_addr, sizeof(server_addr));

    // Set socket timeout
    struct timeval timeout = {0};
    timeout.tv_sec = client->config.timeout_ms / 1000;
    timeout.tv_usec = (client->config.timeout_ms % 1000) * 1000;

    /*
    setsockopt — set the socket options

    int setsockopt(socklen_t optlen;
                      int sockfd, int level, int optname,
                      const void optval[optlen],
                      socklen_t optlen);
    */
    setsockopt(client->sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(client->sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    /*
           #include <fcntl.h>

       int fcntl(int fd, int op, ...);

       DESCRIPTION

       fcntl() performs one of the operations described below on the open
       file descriptor fd.  The operation is determined by op.

       Certain of the operations below are supported only since a
       particular Linux kernel version.  The preferred method of checking
       whether the host kernel supports a particular operation is to
       invoke fcntl() with the desired op value and then test whether the
       call failed with EINVAL, indicating that the kernel does not

       ERRORS        
       EACCES or EAGAIN
              Operation is prohibited by locks held by other processes.

       EAGAIN The operation is prohibited because the file has been
              memory-mapped by another process.

       EBADF  fd is not an open file descriptor

       EINVAL The value specified in op is not recognized by this kernel.         
    */
    int flags = fcntl(client->sockfd, F_GETFL, 0);
    fcntl(client->sockfd, F_SETFL, flags | O_NONBLOCK);

    if (flags == -1) {
        errno = EINVAL;
        return -1;
    }
    // Attempt connection with retries
    uint32_t attempt = 0;
    client_result_t final_result = CLIENT_ERROR_CONNECTION;

    for (attempt = 0; attempt < client->config.max_retries; attempt++)
    {
        int connect_result = connect(client->sockfd, (struct sockaddr *)&client->server_addr,
                                     sizeof(client->server_addr));

        if (connect_result == 0)
        {
            // Restore blocking mode before returning
            fcntl(client->sockfd, F_SETFL, flags);

            client->status = CLIENT_STATUS_CONNECTED;
            client->connect_time = time(NULL);
            client->last_activity = client->connect_time;
            return CLIENT_SUCCESS;
        }

        /*
        The following are general socket errors only.  There may be other
       domain-specific error codes.

       EACCES For UNIX domain sockets, which are identified by pathname:
              Write permission is denied on the socket file, or search
              permission is denied for one of the directories in the path
              prefix.  (See also path_resolution(7).)

       EACCES
       EPERM  The user tried to connect to a broadcast address without
              having the socket broadcast flag enabled or the connection
              request failed because of a local firewall rule.

       EACCES It can also be returned if an SELinux policy denied a
              connection (for example, if there is a policy saying that
              an HTTP proxy can only connect to ports associated with
              HTTP servers, and the proxy tries to connect to a different
              port).

       EADDRINUSE
              Local address is already in use.

       EADDRNOTAVAIL
              (Internet domain sockets) The socket referred to by sockfd
              had not previously been bound to an address and, upon
              attempting to bind it to an ephemeral port, it was
              determined that all port numbers in the ephemeral port
              range are currently in use.  See the discussion of
              /proc/sys/net/ipv4/ip_local_port_range in ip(7).

       EAFNOSUPPORT
              The passed address didn't have the correct address family
              in its sa_family field.

       EAGAIN For nonblocking UNIX domain sockets, the socket is
              nonblocking, and the connection cannot be completed
              immediately.  For other socket families, there are
              insufficient entries in the routing cache.

       EALREADY
              The socket is nonblocking and a previous connection attempt
              has not yet been completed.

       EBADF  sockfd is not a valid open file descriptor.

       ECONNREFUSED
              A connect() on a stream socket found no one listening on
              the remote address.

       EFAULT The socket structure address is outside the user's address
              space.

       EINPROGRESS
              The socket is nonblocking and the connection cannot be
              completed immediately.  (UNIX domain sockets failed with
              EAGAIN instead.)  It is possible to select(2) or poll(2)
              for completion by selecting the socket for writing.  After
              select(2) indicates writability, use getsockopt(2) to read
              the SO_ERROR option at level SOL_SOCKET to determine
              whether connect() completed successfully (SO_ERROR is zero)
              or unsuccessfully (SO_ERROR is one of the usual error codes
              listed here, explaining the reason for the failure).

       EINTR  The system call was interrupted by a signal that was
              caught; see signal(7).

       EISCONN
              The socket is already connected.

       ENETUNREACH
              Network is unreachable.

       ENOTSOCK
              The file descriptor sockfd does not refer to a socket.

       EPROTOTYPE
              The socket type does not support the requested
              communications protocol.  This error can occur, for
              example, on an attempt to connect a UNIX domain datagram
              socket to a stream socket.

       ETIMEDOUT
              Timeout while attempting connection.  The server may be too
              busy to accept new connections.  Note that for IP sockets
              the timeout may be very long when syncookies are enabled on
              the server.
        */
        if (errno == EINPROGRESS || errno == EALREADY)
        {
            // select\poll
            if (check_connection_complete_poll(client->sockfd, client->config.timeout_ms))
            {
                // Restore blocking mode before returning
                fcntl(client->sockfd, F_SETFL, flags);

                client->status = CLIENT_STATUS_CONNECTED;
                client->connect_time = time(NULL);
                client->last_activity = client->connect_time;
                return CLIENT_SUCCESS;
            }
        }
        else if (errno == EISCONN)
        {
            // Restore blocking mode before returning
            fcntl(client->sockfd, F_SETFL, flags);

            client->status = CLIENT_STATUS_CONNECTED;
            client->connect_time = time(NULL);
            client->last_activity = client->connect_time;
            return CLIENT_SUCCESS;
        }
        // Check for temporary errors that are worth retrying
        else if (errno == ECONNREFUSED || errno == ETIMEDOUT ||
                 errno == ENETUNREACH || errno == EHOSTUNREACH)
        {
            // These are temporary errors - continue to next attempt
            final_result = CLIENT_ERROR_CONNECTION;
        }
        else
        {
            // Critical error that shouldn't be retried
            snprintf(client->last_error, sizeof(client->last_error),
                     "Critical connection error: %s", strerror(errno));
            final_result = CLIENT_ERROR_CONNECTION;
            break;
        }

        if (attempt < client->config.max_retries - 1)
        {
            usleep(100000 * (1 << attempt)); // Exponential backoff
        }
    }

    // Restore blocking mode before returning error
    fcntl(client->sockfd, F_SETFL, flags);

    if (final_result != CLIENT_SUCCESS)
    {
        snprintf(client->last_error, sizeof(client->last_error),
                 "Connection failed after %d attempts: %s",
                 client->config.max_retries, strerror(errno));
        close(client->sockfd);
        client->sockfd = -1;
    }

    return final_result;
}

/*
select() allows a program to monitor multiple file descriptors,
       waiting until one or more of the file descriptors become "ready"
       for some class of I/O operation (e.g., input possible).  A file
       descriptor is considered ready if it is possible to perform a
       corresponding I/O operation (e.g., read(2), or a sufficiently
       small write(2)) without blocking.

int select(int nfds, fd_set *_Nullable restrict readfds,
           fd_set *_Nullable restrict writefds,
           fd_set *_Nullable restrict exceptfds,
           struct timeval *_Nullable restrict timeout);

void FD_CLR(int fd, fd_set *set);
int FD_ISSET(int fd, fd_set *set);
void FD_SET(int fd, fd_set *set);
void FD_ZERO(fd_set *set);

int pselect(int nfds, fd_set *_Nullable restrict readfds,
            fd_set *_Nullable restrict writefds,
            fd_set *_Nullable restrict exceptfds,
            const struct timespec *_Nullable restrict timeout,
            const sigset_t *_Nullable restrict sigmask);

   Feature Test Macro Requirements for glibc (see
   feature_test_macros(7)):

       pselect():
           _POSIX_C_SOURCE >= 200112L

WARNING: select() can monitor only file descriptors numbers that
       are less than FD_SETSIZE (1024)—an unreasonably low limit for many
       modern applications—and this limitation will not change.  All
       modern applications should instead use poll(2) or epoll(7), which
       do not suffer this limitation.

---------------------------------------------------------------------------
   fd_set
       A structure type that can represent a set of file descriptors.
       According to POSIX, the maximum number of file descriptors in an
       fd_set structure is the value of the macro FD_SETSIZE.
*/
// static bool check_connection_complete(int sockfd, int timeout_ms)
// {
//     fd_set write_fds, error_fds;
//     struct timeval tv = {0};
//     int result = -1;

//     // MACROS WARNING
//     // void FD_ZERO(fd_set *set);
//     /*
//     This macro clears (removes all file descriptors from) set.
//               It should be employed as the first step in initializing a
//               file descriptor set
//     */
//     FD_ZERO(&write_fds);
//     FD_ZERO(&error_fds);
//     // void FD_SET(int fd, fd_set *set);
//     FD_SET(sockfd, &write_fds);
//     FD_SET(sockfd, &error_fds);

//     tv.tv_sec = timeout_ms / 1000;           // Integer number of seconds
//     tv.tv_usec = (timeout_ms % 1000) * 1000; // Remainder in microseconds
// }

// ▶ but I will use poll, it is more modern and more controlled.

/*
int poll(struct pollfd *fds, nfds_t nfds, int timeout);

#define _GNU_SOURCE         //  See feature_test_macros(7)
#include <poll.h>

int ppoll(struct pollfd *fds, nfds_t nfds,
          const struct timespec *_Nullable tmo_p,
          const sigset_t *_Nullable sigmask);

poll() performs a similar task to select(2): it waits for one of a
       set of file descriptors to become ready to perform I/O.  The
       Linux-specific epoll(7) API performs a similar task, but offers
       features beyond those found in poll().

       The set of file descriptors to be monitored is specified in the
       fds argument, which is an array of structures of the following
       form:

        struct pollfd {
            int   fd;         / file descriptor /
            short events;  / requested events /
            short revents; / returned events /
        }
*/
static bool check_connection_complete_poll(int sockfd, int timeout_ms)
{
    struct pollfd pfd = {0};
    int result = -1;

    pfd.fd = sockfd;
    /*
        POLLOUT
            Writing is now possible, though a write larger than the
            available space in a socket or pipe will still block
            (unless O_NONBLOCK is set).
    */
    pfd.events = POLLOUT; // We are waiting for the socket to become writable
    pfd.revents = 0;

    // int poll(struct pollfd *fds, nfds_t nfds, int timeout);
    result = poll(&pfd, 1, timeout_ms);

    if (result < 0 || result == 0)
    {
        return false;
    }

    /*
    The field *EVENTS* is an input parameter, a bit mask specifying the
       events the application is interested in for the file descriptor
       fd.  This field may be specified as zero, in which case the only
       events that can be returned in *REVENTS* are POLLHUP, POLLERR, and
       POLLNVAL (see below).

    The field revents is an output parameter, filled by the kernel
       with the events that actually occurred.  The bits returned in
       revents can include any of those specified in events, or one of
       the values POLLERR, POLLHUP, or POLLNVAL.  (These three bits are
       meaningless in the events field, and will be set in the revents
       field whenever the corresponding condition is true.)
    */
    if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL))
    {
        return false;
    }

    if (pfd.revents & POLLOUT)
    {
        int so_error = 0;
        socklen_t len = sizeof(so_error);

        /*
        int getsockopt(socklen *restrict optlen;
                      int sockfd, int level, int optname,
                      void optval[_Nullable restrict *optlen],
                      socklen_t *restrict optlen);
        */
        if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &so_error, &len) < 0)
        {
            return false;
        }

        return (so_error == 0);
    }

    return false;
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
