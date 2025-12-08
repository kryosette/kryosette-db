/**
 * @file command_errors.h
 * @brief Command execution error codes and utilities
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// ==================== Error Code Getters ====================

int get_error_success(void);
int get_error_system_not_initialized(void);
int get_error_invalid_parameters(void);
int get_error_command_not_found(void);
int get_error_command_corrupted(void);
int get_error_command_invalid(void);
int get_error_invalid_arg_count(void);
int get_error_arg_validation_failed(void);
int get_error_no_handler(void);

// ==================== Error Message Getters ====================

const char* get_error_message(int error_code);
const char* get_error_message_success(void);
const char* get_error_message_system_not_initialized(void);
const char* get_error_message_invalid_parameters(void);
const char* get_error_message_command_not_found(void);
const char* get_error_message_command_corrupted(void);
const char* get_error_message_command_invalid(void);
const char* get_error_message_invalid_arg_count(void);
const char* get_error_message_arg_validation_failed(void);
const char* get_error_message_no_handler(void);

// ==================== Utility Functions ====================

int is_error_code(int code);
int is_success_code(int code);
const char* error_code_to_string(int code);

#ifdef __cplusplus
}
#endif