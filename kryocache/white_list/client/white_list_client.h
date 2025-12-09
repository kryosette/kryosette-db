#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct client_instance client_instance_t;
typedef void* secure_cmd_id_t;
typedef void* enum_system_t;

typedef struct command_definition_impl* command_definition_t;
typedef struct enum_system_impl* enum_system_impl_t;

int command_system_global_init(uint64_t seed);
void command_system_global_cleanup(void);
int is_command_system_initialized(void);

command_definition_t get_command_secure(const char* cmd_name);
int execute_command_safely(client_instance_t *client, 
                          const char* cmd_name,
                          const char **args, 
                          size_t args_count);

enum_system_t enum_system_init(uint64_t seed);
enum_system_t cmd_system_init(uint64_t seed);
void enum_system_destroy(enum_system_t sys);
int secure_validate_cmd_id(enum_system_t sys, secure_cmd_id_t cmd_id);

void safe_to_upper_string(char *dest, size_t dest_size, const char *src);

const char* cmd_def_get_name(const struct command_definition_impl *cmd);
secure_cmd_id_t cmd_def_get_id(const struct command_definition_impl *cmd);
size_t cmd_def_get_min_args(const struct command_definition_impl *cmd);
size_t cmd_def_get_max_args(const struct command_definition_impl *cmd);
bool cmd_def_has_validator(const struct command_definition_impl *cmd);
bool cmd_def_validate(const struct command_definition_impl *cmd, const char **args, size_t args_count);
void cmd_def_execute(const struct command_definition_impl *cmd, client_instance_t *client, const char **args, size_t args_count);
uint32_t cmd_def_get_sec_front(const struct command_definition_impl *cmd);
uint32_t cmd_def_get_sec_back(const struct command_definition_impl *cmd);
bool cmd_def_check_integrity(const struct command_definition_impl *cmd);