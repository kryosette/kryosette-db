#include "white_list_client.h"
#include <stdio.h>
#include <string.h>

// ==================== Валидаторы ====================
bool validate_key(const char** args, size_t args_count) {
    return args_count == 1 && args[0] && strlen(args[0]) > 0;
}

bool validate_kv(const char** args, size_t args_count) {
    return args_count == 2 && args[0] && args[1] && strlen(args[0]) > 0;
}

bool validate_keys(const char** args, size_t args_count) {
    if (args_count < 1 || args_count > 5) return false;
    for (size_t i = 0; i < args_count; i++) {
        if (!args[i] || strlen(args[i]) == 0) return false;
    }
    return true;
}

bool validate_auth(const char** args, size_t args_count) {
    return args_count == 1 && args[0] && strlen(args[0]) > 0;
}

// ==================== Функции ошибок ====================
int get_error_system_not_initialized(void) { return -1; }
int get_error_invalid_parameters(void) { return -2; }
int get_error_command_not_found(void) { return -3; }
int get_error_command_corrupted(void) { return -4; }
int get_error_command_invalid(void) { return -5; }
int get_error_invalid_arg_count(void) { return -6; }
int get_error_arg_validation_failed(void) { return -7; }
int get_error_success(void) { return 0; }
int get_error_no_handler(void) { return -8; }
int get_error_invalid_context(void) { return -9; }
int get_error_access_denied(void) { return -10; }

// ==================== send_response_to_client заглушка ====================
void send_response_to_client(client_instance_t* client, const char* response) {
    if (client && response) {
        printf("[RESPONSE TO CLIENT %p]: %s\n", (void*)client, response);
    }
}

// ==================== cmd_system_init заглушка ====================
enum_system_t cmd_system_init(uint64_t seed) {
    return enum_system_init(seed);
}