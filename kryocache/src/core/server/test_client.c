#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void send_command(const char *host, int port, const char *command)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, host, &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("connect failed");
        return;
    }

    // Отправляем команду
    write(sock, command, strlen(command));
    write(sock, "\n", 1);

    // Читаем ответ
    char buffer[1024];
    int bytes_read = read(sock, buffer, sizeof(buffer) - 1);
    if (bytes_read > 0)
    {
        buffer[bytes_read] = '\0';
        printf("Server response: %s", buffer);
    }

    close(sock);
}

int main()
{
    printf("Testing cache server...\n");

    // Тестовые команды
    send_command("127.0.0.1", 6898, "PING");
    send_command("127.0.0.1", 6898, "SET user_id 123");
    send_command("127.0.0.1", 6898, "GET user_id");
    send_command("127.0.0.1", 6898, "SET counter 100 60"); // TTL 60 секунд
    send_command("127.0.0.1", 6898, "GET counter");
    send_command("127.0.0.1", 6898, "INFO");
    send_command("127.0.0.1", 6898, "DEL user_id");
    send_command("127.0.0.1", 6898, "GET user_id"); // Должен вернуть ошибку

    return 0;
}