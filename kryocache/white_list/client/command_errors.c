/**
 * @file command_errors.c
 * @brief Command execution error codes implementation
 */

#include "command_errors.h"
#include <string.h>

// ==================== Error Code Constants ====================

static const int ERROR_SUCCESS = 0;
static const int ERROR_SYSTEM_NOT_INITIALIZED = -1;
static const int ERROR_INVALID_PARAMETERS = -2;
static const int ERROR_COMMAND_NOT_FOUND = -3;
static const int ERROR_COMMAND_CORRUPTED = -4;
static const int ERROR_COMMAND_INVALID = -5;
static const int ERROR_INVALID_ARG_COUNT = -6;
static const int ERROR_ARG_VALIDATION_FAILED = -7;
static const int ERROR_NO_HANDLER = -8;

// ==================== Error Message Constants ====================

static const char* ERROR_MSG_SUCCESS = "Success";
static const char* ERROR_MSG_SYSTEM_NOT_INITIALIZED = "Command system not initialized";
static const char* ERROR_MSG_INVALID_PARAMETERS = "Invalid parameters provided";
static const char* ERROR_MSG_COMMAND_NOT_FOUND = "Command not found";
static const char* ERROR_MSG_COMMAND_CORRUPTED = "Command structure corrupted";
static const char* ERROR_MSG_COMMAND_INVALID = "Invalid command ID detected";
static const char* ERROR_MSG_INVALID_ARG_COUNT = "Invalid number of arguments";
static const char* ERROR_MSG_ARG_VALIDATION_FAILED = "Argument validation failed";
static const char* ERROR_MSG_NO_HANDLER = "No handler for command";

// ==================== Error Code Getters ====================

int get_error_success(void) { return ERROR_SUCCESS; }
int get_error_system_not_initialized(void) { return ERROR_SYSTEM_NOT_INITIALIZED; }
int get_error_invalid_parameters(void) { return ERROR_INVALID_PARAMETERS; }
int get_error_command_not_found(void) { return ERROR_COMMAND_NOT_FOUND; }
int get_error_command_corrupted(void) { return ERROR_COMMAND_CORRUPTED; }
int get_error_command_invalid(void) { return ERROR_COMMAND_INVALID; }
int get_error_invalid_arg_count(void) { return ERROR_INVALID_ARG_COUNT; }
int get_error_arg_validation_failed(void) { return ERROR_ARG_VALIDATION_FAILED; }
int get_error_no_handler(void) { return ERROR_NO_HANDLER; }

// ==================== Error Message Getters ====================

const char* get_error_message_success(void) { return ERROR_MSG_SUCCESS; }
const char* get_error_message_system_not_initialized(void) { return ERROR_MSG_SYSTEM_NOT_INITIALIZED; }
const char* get_error_message_invalid_parameters(void) { return ERROR_MSG_INVALID_PARAMETERS; }
const char* get_error_message_command_not_found(void) { return ERROR_MSG_COMMAND_NOT_FOUND; }
const char* get_error_message_command_corrupted(void) { return ERROR_MSG_COMMAND_CORRUPTED; }
const char* get_error_message_command_invalid(void) { return ERROR_MSG_COMMAND_INVALID; }
const char* get_error_message_invalid_arg_count(void) { return ERROR_MSG_INVALID_ARG_COUNT; }
const char* get_error_message_arg_validation_failed(void) { return ERROR_MSG_ARG_VALIDATION_FAILED; }
const char* get_error_message_no_handler(void) { return ERROR_MSG_NO_HANDLER; }

// ==================== Utility Functions ====================

const char* get_error_message(int error_code) {
    switch (error_code) {
        case ERROR_SUCCESS:                    return ERROR_MSG_SUCCESS;
        case ERROR_SYSTEM_NOT_INITIALIZED:     return ERROR_MSG_SYSTEM_NOT_INITIALIZED;
        case ERROR_INVALID_PARAMETERS:         return ERROR_MSG_INVALID_PARAMETERS;
        case ERROR_COMMAND_NOT_FOUND:          return ERROR_MSG_COMMAND_NOT_FOUND;
        case ERROR_COMMAND_CORRUPTED:          return ERROR_MSG_COMMAND_CORRUPTED;
        case ERROR_COMMAND_INVALID:            return ERROR_MSG_COMMAND_INVALID;
        case ERROR_INVALID_ARG_COUNT:          return ERROR_MSG_INVALID_ARG_COUNT;
        case ERROR_ARG_VALIDATION_FAILED:      return ERROR_MSG_ARG_VALIDATION_FAILED;
        case ERROR_NO_HANDLER:                 return ERROR_MSG_NO_HANDLER;
        default:                               return "Unknown error code";
    }
}

int is_error_code(int code) {
    return code < ERROR_SUCCESS;
}

int is_success_code(int code) {
    return code == ERROR_SUCCESS;
}

const char* error_code_to_string(int code) {
    switch (code) {
        case ERROR_SUCCESS:                    return "SUCCESS";
        case ERROR_SYSTEM_NOT_INITIALIZED:     return "ERROR_SYSTEM_NOT_INITIALIZED";
        case ERROR_INVALID_PARAMETERS:         return "ERROR_INVALID_PARAMETERS";
        case ERROR_COMMAND_NOT_FOUND:          return "ERROR_COMMAND_NOT_FOUND";
        case ERROR_COMMAND_CORRUPTED:          return "ERROR_COMMAND_CORRUPTED";
        case ERROR_COMMAND_INVALID:            return "ERROR_COMMAND_INVALID";
        case ERROR_INVALID_ARG_COUNT:          return "ERROR_INVALID_ARG_COUNT";
        case ERROR_ARG_VALIDATION_FAILED:      return "ERROR_ARG_VALIDATION_FAILED";
        case ERROR_NO_HANDLER:                 return "ERROR_NO_HANDLER";
        default:                               return "ERROR_UNKNOWN";
    }
}