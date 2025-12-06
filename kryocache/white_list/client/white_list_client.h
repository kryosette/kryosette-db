#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef void* secure_cmd_id_t;   
typedef void* enum_system_t;   

// typedef struct {
//     enum_system_t cmd_system;  // Добавляем систему команд
//     // ... другие поля
// } client_instance_t;

enum_system_t cmd_system_init(uint64_t seed);
void cmd_system_destroy(enum_system_t sys);

secure_cmd_id_t cmd_system_get_cmd(enum_system_t sys, const char* cmd_name);
int cmd_system_validate(enum_system_t sys, secure_cmd_id_t cmd_id);

bool validate_key(const char **args, size_t args_count);
bool validate_kv(const char **args, size_t args_count);
bool validate_keys(const char **args, size_t args_count);
bool validate_auth(const char **args, size_t args_count);

void handle_get(client_instance_t *client, const char **args, size_t args_count);
void handle_set(client_instance_t *client, const char **args, size_t args_count);
void handle_delete(client_instance_t *client, const char **args, size_t args_count);
void handle_ping(client_instance_t *client, const char **args, size_t args_count);
void handle_quit(client_instance_t *client, const char **args, size_t args_count);
void handle_auth(client_instance_t *client, const char **args, size_t args_count);