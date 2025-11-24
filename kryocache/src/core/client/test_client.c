#include "client.h"
#include <stdio.h>
#include <unistd.h>

int main()
{
    printf("🧪 Testing Kryocache Client Library\n\n");

    // 1. Инициализация клиента
    printf("1. Initializing client...\n");
    client_instance_t *client = client_init_default();
    if (!client)
    {
        printf("❌ Failed to initialize client\n");
        return 1;
    }
    printf("✅ Client initialized successfully\n\n");

    // 2. Подключение к серверу
    printf("2. Connecting to server...\n");
    client_result_t result = client_connect(client);
    if (result == CLIENT_SUCCESS)
    {
        printf("✅ Connected to server successfully!\n\n");

        // 3. Тестируем PING
        printf("3. Testing PING command...\n");
        result = client_ping(client);
        if (result == CLIENT_SUCCESS)
        {
            printf("✅ Server is responsive\n\n");
        }
        else
        {
            printf("❌ Ping failed: %s\n\n", client_get_last_error(client));
        }

        // 4. Тестируем SET/GET
        printf("4. Testing SET/GET commands...\n");
        result = client_set(client, "test_key", "test_value_123");
        if (result == CLIENT_SUCCESS)
        {
            printf("✅ SET command successful\n");

            char value[256];
            result = client_get(client, "test_key", value, sizeof(value));
            if (result == CLIENT_SUCCESS)
            {
                printf("✅ GET command successful: %s\n\n", value);
            }
            else
            {
                printf("❌ GET failed: %s\n\n", client_get_last_error(client));
            }
        }
        else
        {
            printf("❌ SET failed: %s\n\n", client_get_last_error(client));
        }

        // 5. Показываем статистику
        printf("5. Client statistics:\n");
        client_stats_t stats;
        if (client_get_stats(client, &stats))
        {
            printf("   📊 Operations: %lu total, %lu failed\n",
                   stats.operations_total, stats.operations_failed);
            printf("   📡 Bytes: %lu sent, %lu received\n",
                   stats.bytes_sent, stats.bytes_received);
            printf("   🔄 Reconnects: %u\n", stats.reconnect_count);
        }

        // 6. Отключаемся
        client_disconnect(client);
        printf("\n✅ Disconnected from server\n");
    }
    else
    {
        printf("❌ Connection failed: %s\n", client_get_last_error(client));
        printf("💡 Make sure the server is running on [::1]:6898\n");
    }

    // 7. Очистка
    client_destroy(client);
    printf("\n🎉 Test completed successfully!\n");

    return 0;
}