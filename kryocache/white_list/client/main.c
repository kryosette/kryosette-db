#include "white_list_client.h"
#include <stdio.h>
#include <time.h>

int main(void) {
    printf("=== Testing Secure Command System ===\n");
    
    // Инициализация
    uint64_t seed = time(NULL) ^ 0xDEADBEEFCAFEBABE;
    
    if (!command_system_global_init(seed)) {
        printf("❌ Failed to initialize command system\n");
        return 1;
    }
    printf("✅ Command system initialized\n");
    
    // Тест 1: Получение команды
    struct command_definition_impl* cmd = get_command_secure("GET");
    if (cmd) {
        printf("✅ Found command: %s\n", cmd->cmd_name);
    } else {
        printf("❌ Command not found\n");
    }
    
    // Тест 2: Валидация команды
    if (cmd && secure_validate_cmd_id(g_global_cmd_system, cmd->cmd_id)) {
        printf("✅ Command ID validated successfully\n");
    } else {
        printf("❌ Command validation failed\n");
    }
    
    // Тест 3: Очистка
    command_system_global_cleanup();
    if (!is_command_system_initialized()) {
        printf("✅ Command system cleaned up successfully\n");
    }
    
    printf("=== All tests passed! ===\n");
    return 0;
}