#include "white_list_client.h"
#include "/Users/dimaeremin/kryosette-db/third-party/drs-generator/src/core/drs_generator.h"
// #include "/Users/dimaeremin/kryosette-db/third-party/smemset/include/smemset.h"
#include "command_errors.h"

#include <stdlib.h>   
#include <string.h>  
#include <time.h>
#include <stdio.h>

#include "/Users/dimaeremin/kryosette-db/kryocache/src/core/client/include/client.h"

// temp
extern bool validate_key(const char** args, size_t args_count);
extern bool validate_kv(const char** args, size_t args_count);
extern bool validate_keys(const char** args, size_t args_count);
extern bool validate_auth(const char** args, size_t args_count);
extern void send_response_to_client(client_instance_t* client, const char* response);
extern client_result_t client_get(client_instance_t* client, const char* key, char* buffer, size_t size);
extern client_result_t client_set(client_instance_t* client, const char* key, const char* value);
extern client_result_t client_delete(client_instance_t* client, const char* key);
extern client_result_t client_ping(client_instance_t* client);
extern client_result_t client_exists(client_instance_t* client, const char* key);

static void handle_get(client_instance_t* client, const char** args, size_t args_count);
static void handle_set(client_instance_t* client, const char** args, size_t args_count);
static void handle_delete(client_instance_t* client, const char** args, size_t args_count);
static void handle_ping(client_instance_t* client, const char** args, size_t args_count);
static void handle_quit(client_instance_t* client, const char** args, size_t args_count);
static void handle_auth(client_instance_t* client, const char** args, size_t args_count);
static void handle_exists(client_instance_t* client, const char** args, size_t args_count);
static void handle_keys(client_instance_t* client, const char** args, size_t args_count);
static void handle_info(client_instance_t* client, const char** args, size_t args_count);
static void handle_select(client_instance_t* client, const char** args, size_t args_count);

enum white_list {
    CMD_GET = 1,
    CMD_SET = 2,
    CMD_DELETE = 3,
    CMD_EXISTS = 4,
    CMD_KEYS = 5,
    CMD_PING = 6,
    CMD_INFO = 7,
    CMD_QUIT = 8,
    CMD_AUTH = 9,
    CMD_SELECT = 10,
    CMD_MAX_VALID = 11 
};

//                                 (2) number of columns
static const uint64_t CMD_SEEDS[10][2] = { 
    {0xDEADBEEF12345678ULL, 0xCAFEBABE87654321ULL}, // GET
    {0xBEEFDEAD56781234ULL, 0xFACEFACE43218765ULL}, // SET
    {0xFEEDFACE87654321ULL, 0xDECAFBAD12345678ULL}, // DELETE
    {0xCAFEDEAD43218765ULL, 0xBEEFFACE56781234ULL}, // EXISTS
    {0xDEADC0DE56781234ULL, 0xFACEB00C43218765ULL}, // KEYS
    {0xBADCAFE87654321ULL, 0xDEADC0DE12345678ULL},  // PING
    {0xC0FFEE12345678ULL, 0xFEABA687654321ULL},     // INFO
    {0xBABEFACE43218765ULL, 0xDEADD00D56781234ULL}, // QUIT
    {0xFACEFEED87654321ULL, 0xCAFEB0BA12345678ULL}, // AUTH
    {0xDEADFACE56781234ULL, 0xFEEDBEEF43218765ULL}, // SELECT
};

static const char *CMD_NAMES[10] = {
    "GET", "SET", "DELETE", "EXISTS", "KEYS",
    "PING", "INFO", "QUIT", "AUTH", "SELECT"
};

struct drs_generator {
    uint64_t state[2];
};

struct enum_system_impl
{
    struct drs_generator gen;
    secure_cmd_id_t generated_ids[10];
    uint64_t validation_keys[10];
    time_t init_time;
    uint32_t system_magic;
};

struct command_definition_impl {
    const char *cmd_name;
    secure_cmd_id_t cmd_id;   
    size_t min_args;
    size_t max_args;
    bool (*validator)(const char **args, size_t args_count);
    void (*handler)(client_instance_t *client, const char **args, size_t args_count);
    
    uint32_t sec_front;
    uint32_t sec_back;
};

static enum_system_t g_global_cmd_system = NULL;
static struct command_definition_impl g_valid_commands[6] = {0};
static int g_command_system_initialized = 0;

int command_system_global_init(uint64_t seed) {
    if (!seed) return 0;

    if (g_command_system_initialized) {
        return 1;
    }

    g_global_cmd_system = enum_system_init(seed);
    if (!g_global_cmd_system) return 0;

    const struct {
        const char* name;
        size_t min_args;
        size_t max_args;
        bool (*validator)(const char** args, size_t);
        void (*handler)(client_instance_t*, const char** args, size_t);
    } cmd_templates[6] = {
        {"GET",    1, 1, validate_key,   handle_get},
        {"SET",    2, 2, validate_kv,    handle_set},
        {"DELETE",    1, 5, validate_keys,  handle_delete},
        {"PING",   0, 0, NULL,           handle_ping},
        {"QUIT",   0, 0, NULL,           handle_quit},
        {"AUTH",   1, 1, validate_auth,  handle_auth},
    };

    for (int i = 0; i < 6; i++) {
        secure_cmd_id_t cmd_id = NULL;
        struct enum_system_impl *sys = (struct enum_system_impl*)g_global_cmd_system;

        for (int j = 0; j < 10; j++) {
            /*
            int strncmp(const char *s1, const char *s2, size_t n);
            */
            if (strcmp(CMD_NAMES[j], cmd_templates[i].name) == 0) {
                cmd_id = sys->generated_ids[j];
                break;
            }
        }

        if (!cmd_id) {
            command_system_global_cleanup();
            return 0;
        }

        g_valid_commands[i].cmd_name = cmd_templates[i].name;
        g_valid_commands[i].cmd_id = cmd_id;
        g_valid_commands[i].min_args = cmd_templates[i].min_args;
        g_valid_commands[i].max_args = cmd_templates[i].max_args;
        g_valid_commands[i].validator = cmd_templates[i].validator;
        g_valid_commands[i].handler = cmd_templates[i].handler;
        g_valid_commands[i].sec_front = 0x434D4453;  // "CMDS"
        g_valid_commands[i].sec_back = 0x53454355; // "SECU"
    }

    g_command_system_initialized = 1;

    return 1;
}

void safe_to_upper_string(char *dest, size_t dest_size, const char *src) {
    if (!dest || !src || dest_size <= 1) return;

    size_t i = 0;
    for (i = 0; i < dest_size - 1 && src[i] != '\0'; i++) {
        unsigned char c = src[i];
        if (c >= 'a' && c <= 'z') {
            dest[i] = c - 32;  
        } else {
            dest[i] = c;
        }
    }

    if (i < dest_size) {
        dest[i] = '\0';
    } else if (dest_size > 0) {
        dest[dest_size - 1] = '\0';
    }
}

static secure_cmd_id_t generate_secure_id(struct drs_generator *gen, int cmd_index) {
    uint64_t base = drs_next(gen);

    /*
    Why 0x9e3779b9?

    This is an approximation of the golden ratio *2^32
    Widely used in hash functions (e.g. murmurhash)
    Good scattering properties (avalanche effect)
    */
    uint64_t transformed = base ^ (cmd_index * 0x9e3779b9ULL);

    int shift = (base % 61) + 3; // From 3 to 63 bits
    /*
    (transformed << shift) - left shift by shift bit
    (transformed >> (64 - shift)) - shift to the right by the remaining bits
    */
    transformed = (transformed << shift) | (transformed >> (64 - shift));

    /*
    Generating another random number via DRS
    Range 0x10000000 (268,435,456) to 0xFFFFFFFF (4,294,967,295)
    We use XOR for additional "mixing"
    */
    uint64_t magic = drs_range(gen, 0x10000000ULL, 0xFFFFFFFFULL);
    transformed ^= magic;

    /*
    Purpose: To hide the real data type (uint64_t) behind an opaque pointer (void*)

    in white_list_client.h - typedef void* secure_cmd_id_t;   
    */
    return (secure_cmd_id_t)(uintptr_t)transformed; // to void
}

enum_system_t enum_system_init(uint64_t seed) {
    struct enum_system_impl *sys = calloc(1, sizeof(*sys));
    if (!sys) return NULL;

    uint64_t seed1 = seed ^ 0xDEADBEEFCAFEBABE;

    /*
    '~' - inversion
    */
    uint64_t seed2 = ~seed ^ 0xBEEFDEADFACEFACE;
    drs_init(&sys->gen, seed1, seed2); 

    sys->init_time = time(NULL);
    // "ENUMSEC\0" (ASCII)
    sys->system_magic = 0x454E554D53454300;

    for (int i = 0; i < 10; i++) {
        struct drs_generator cmd_gen;
        drs_init(&cmd_gen, CMD_SEEDS[i][0], CMD_SEEDS[i][1]);

        sys->generated_ids[i] = generate_secure_id(&cmd_gen, i);
    
        sys->validation_keys[i] = drs_range(&cmd_gen, 0x1000ULL, 0xFFFFULL);
        uint64_t id_value = (uint64_t)(uintptr_t)sys->generated_ids[i];
        sys->validation_keys[i] ^= id_value;

        /*
        example:
        0x3344556677880000 (16)
        and the remaining 48 bits
        */
        sys->validation_keys[i] = (sys->validation_keys[i] << 16) | 
                              (sys->validation_keys[i] >> 48);
    }

    return (enum_system_t)sys;
}

int secure_validate_cmd_id(enum_system_t sys, secure_cmd_id_t cmd_id) {
    struct enum_system_impl *enum_sys = (struct enum_system_impl*)sys;
    if (!enum_sys || enum_sys->system_magic != 0x454E554D53454300) return 0;

    /*
    `uintptr_t` is an unsigned integer type defined in the C and C++ standards (specifically C99 and C++11 and later). 
    Its primary purpose is to provide an integer type that is guaranteed to be large enough to hold the value of any void* pointer and, 
    when converted back to void*, will yield a pointer equal to the original.
    */
    uint64_t cmd_value = (uint64_t)(uintptr_t)cmd_id;

    for (int i = 0; i < 10; i++) {
        uint64_t gen_id_value = (uint64_t)(uintptr_t)enum_sys->generated_ids[i];
        if (gen_id_value == cmd_value) {
            uint64_t check = cmd_value ^ enum_sys->validation_keys[i];
            check = (check >> 16) | (check << 48);
            if ((check & 0xFFFF) == (enum_sys->validation_keys[i] & 0xFFFF)) {
                return 1;
            }
        }
    }

    return 0;
}

void enum_system_destroy(enum_system_t sys) {
    if (!sys) return;

    struct enum_system_impl *enum_sys = (struct enum_system_impl*)sys;

    memset(enum_sys->generated_ids, 0, sizeof(enum_sys->generated_ids));
    memset(enum_sys->validation_keys, 0, sizeof(enum_sys->validation_keys));

    free(enum_sys);
}

void command_system_global_cleanup(void) {
    if (g_global_cmd_system) {
        enum_system_destroy(g_global_cmd_system);
        g_global_cmd_system = NULL;
    }
    
    memset(g_valid_commands, 0, sizeof(g_valid_commands));
    g_command_system_initialized = 0;
}

int is_command_system_initialized(void) {
    return g_command_system_initialized;
}

struct command_definition_impl* get_command_secure(const char* cmd_name) {
    if (!g_command_system_initialized || !cmd_name) return NULL;
    
    char upper_cmd[32];
    strncpy(upper_cmd, cmd_name, sizeof(upper_cmd) - 1);
    upper_cmd[sizeof(upper_cmd) - 1] = '\0';
    safe_to_upper_string(upper_cmd, sizeof(upper_cmd), upper_cmd);
    
    for (int i = 0; i < 6; i++) {
        if (g_valid_commands[i].sec_front != 0x434D4453 ||
            g_valid_commands[i].sec_back != 0x53454355) {
            continue;
        }
        
        if (strcmp(g_valid_commands[i].cmd_name, upper_cmd) == 0) {
            return &g_valid_commands[i];
        }
    }
    
    return NULL;
}

int execute_command_safely(client_instance_t *client, 
                          const char* cmd_name,
                          const char **args, 
                          size_t args_count) {
    if (!is_command_system_initialized()) {
        return get_error_system_not_initialized();
    }
    
    if (!client || !cmd_name || !args) {
        return get_error_invalid_parameters();
    }
    
    struct command_definition_impl* cmd = get_command_secure(cmd_name);
    if (!cmd) {
        return get_error_command_not_found();
    }
    
    if (cmd->sec_front != 0x434D4453 || cmd->sec_back != 0x53454355) {
        return get_error_command_corrupted();
    }
    
    if (!secure_validate_cmd_id(g_global_cmd_system, cmd->cmd_id)) {
        return get_error_command_invalid();
    }
    
    if (args_count < cmd->min_args || args_count > cmd->max_args) {
        return get_error_invalid_arg_count();
    }
    
    if (cmd->validator && !cmd->validator(args, args_count)) {
        return get_error_arg_validation_failed();
    }
    
    if (cmd->handler) {
        cmd->handler(client, args, args_count);
        return get_error_success();
    }
    
    return get_error_no_handler();
}

const char* cmd_def_get_name(const struct command_definition_impl *cmd) {
    return cmd ? cmd->cmd_name : NULL;
}

secure_cmd_id_t cmd_def_get_id(const struct command_definition_impl *cmd) {
    return cmd ? cmd->cmd_id : NULL;
}

size_t cmd_def_get_min_args(const struct command_definition_impl *cmd) {
    return cmd ? cmd->min_args : 0;
}

size_t cmd_def_get_max_args(const struct command_definition_impl *cmd) {
    return cmd ? cmd->max_args : 0;
}

bool cmd_def_has_validator(const struct command_definition_impl *cmd) {
    return cmd && cmd->validator != NULL;
}

bool cmd_def_validate(const struct command_definition_impl *cmd, const char **args, size_t args_count) {
    if (!cmd || !cmd->validator) return true; 
    return cmd->validator(args, args_count);
}

void cmd_def_execute(const struct command_definition_impl *cmd, client_instance_t *client, const char **args, size_t args_count) {
    if (cmd && cmd->handler) {
        cmd->handler(client, args, args_count);
    }
}

uint32_t cmd_def_get_sec_front(const struct command_definition_impl *cmd) {
    return cmd ? cmd->sec_front : 0;
}

uint32_t cmd_def_get_sec_back(const struct command_definition_impl *cmd) {
    return cmd ? cmd->sec_back : 0;
}

bool cmd_def_check_integrity(const struct command_definition_impl *cmd) {
    if (!cmd) return false;
    return (cmd->sec_front == 0x434D4453 && cmd->sec_back == 0x53454355); // "CMDS" и "SECU"
}

// ==================== ОБРАБОТЧИКИ КОМАНД ====================

static void handle_get(client_instance_t* client, const char** args, size_t args_count) {
    if (!client || !args || args_count < 1) {
        return;
    }
    
    struct command_definition_impl* cmd = get_command_secure("GET");
    if (!cmd) return;
    
    if (cmd->sec_front != 0x434D4453 || cmd->sec_back != 0x53454355) {
        return;
    }
    
    if (!secure_validate_cmd_id(g_global_cmd_system, cmd->cmd_id)) {
        return;
    }
    
    if (cmd->validator && !cmd->validator(args, args_count)) {
        return;
    }
    
    const char* key = args[0];
    char buffer[4096];
    
    client_result_t result = client_get(client, key, buffer, sizeof(buffer));
    
    if (result == CLIENT_SUCCESS) {
        if (strlen(buffer) > 0) {
            char response[4096 + 64];
            snprintf(response, sizeof(response), "\"%s\"", buffer);
            send_response_to_client(client, response);
        } else {
            send_response_to_client(client, "(nil)");
        }
    } else {
        send_response_to_client(client, "(error)");
    }
}

static void handle_set(client_instance_t* client, const char** args, size_t args_count) {
    if (!client || !args || args_count < 2) {
        return;
    }
    
    struct command_definition_impl* cmd = get_command_secure("SET");
    if (!cmd) return;
    
    if (cmd->sec_front != 0x434D4453 || cmd->sec_back != 0x53454355) {
        return;
    }
    
    if (!secure_validate_cmd_id(g_global_cmd_system, cmd->cmd_id)) {
        return;
    }
    
    if (cmd->validator && !cmd->validator(args, args_count)) {
        return;
    }
    
    const char* key = args[0];
    const char* value = args[1];
    
    client_result_t result = client_set(client, key, value);
    
    if (result == CLIENT_SUCCESS) {
        send_response_to_client(client, "OK");
    } else {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "(error: %d)", result);
        send_response_to_client(client, error_msg);
    }
}

static void handle_delete(client_instance_t* client, const char** args, size_t args_count) {
    if (!client || !args || args_count < 1) {
        return;
    }
    
    struct command_definition_impl* cmd = get_command_secure("DELETE");
    if (!cmd) return;
    
    if (cmd->sec_front != 0x434D4453 || cmd->sec_back != 0x53454355) {
        return;
    }
    
    if (!secure_validate_cmd_id(g_global_cmd_system, cmd->cmd_id)) {
        return;
    }
    
    if (cmd->validator && !cmd->validator(args, args_count)) {
        return;
    }
    
    int deleted_count = 0;
    
    for (size_t i = 0; i < args_count; i++) {
        client_result_t result = client_delete(client, args[i]);
        if (result == CLIENT_SUCCESS) {
            deleted_count++;
        }
    }
    
    char response[64];
    snprintf(response, sizeof(response), "(%d)", deleted_count);
    send_response_to_client(client, response);
}

static void handle_ping(client_instance_t* client, const char** args, size_t args_count) {
    if (!client) {
        return;
    }
    
    struct command_definition_impl* cmd = get_command_secure("PING");
    if (!cmd) return;
    
    if (cmd->sec_front != 0x434D4453 || cmd->sec_back != 0x53454355) {
        return;
    }
    
    if (!secure_validate_cmd_id(g_global_cmd_system, cmd->cmd_id)) {
        return;
    }
    
    // PING может иметь 0 или 1 аргумент
    if (args_count > 1) {
        send_response_to_client(client, "ERR wrong number of arguments for 'ping' command");
        return;
    }
    
    client_result_t result = client_ping(client);
    
    if (result == CLIENT_SUCCESS) {
        if (args_count == 1 && args[0]) {
            send_response_to_client(client, args[0]);
        } else {
            send_response_to_client(client, "PONG");
        }
    } else {
        send_response_to_client(client, "(error)");
    }
}

static void handle_quit(client_instance_t* client, const char** args, size_t args_count) {
    if (!client) {
        return;
    }
    
    struct command_definition_impl* cmd = get_command_secure("QUIT");
    if (!cmd) return;
    
    if (cmd->sec_front != 0x434D4453 || cmd->sec_back != 0x53454355) {
        return;
    }
    
    if (!secure_validate_cmd_id(g_global_cmd_system, cmd->cmd_id)) {
        return;
    }
    
    // QUIT не должен иметь аргументов
    if (args_count > 0) {
        send_response_to_client(client, "ERR wrong number of arguments for 'quit' command");
        return;
    }
    
    send_response_to_client(client, "OK");
    
    // Инициируем отключение клиента
    client_disconnect(client);
}

static void handle_auth(client_instance_t* client, const char** args, size_t args_count) {
    if (!client || !args || args_count < 1) {
        return;
    }
    
    struct command_definition_impl* cmd = get_command_secure("AUTH");
    if (!cmd) return;
    
    if (cmd->sec_front != 0x434D4453 || cmd->sec_back != 0x53454355) {
        return;
    }
    
    if (!secure_validate_cmd_id(g_global_cmd_system, cmd->cmd_id)) {
        return;
    }
    
    if (cmd->validator && !cmd->validator(args, args_count)) {
        send_response_to_client(client, "ERR invalid password");
        return;
    }
    
    const char* password = args[0];
    
    // TODO: Реальная аутентификация - сравнение с хэшем или проверка токена
    // Пока простая проверка для примера
    if (strcmp(password, "secret123") == 0) {
        // Заглушка - нужно определить структуру client_instance_t
        // client->authenticated = 1;
        send_response_to_client(client, "OK");
    } else {
        send_response_to_client(client, "ERR invalid password");
    }
}

// ==================== ДОПОЛНИТЕЛЬНЫЕ ОБРАБОТЧИКИ ====================

static void handle_exists(client_instance_t* client, const char** args, size_t args_count) {
    if (!client || !args || args_count < 1) {
        return;
    }
    
    struct command_definition_impl* cmd = get_command_secure("EXISTS");
    if (!cmd) return;
    
    if (cmd->sec_front != 0x434D4453 || cmd->sec_back != 0x53454355) {
        return;
    }
    
    if (!secure_validate_cmd_id(g_global_cmd_system, cmd->cmd_id)) {
        return;
    }
    
    int exists_count = 0;
    
    for (size_t i = 0; i < args_count; i++) {
        client_result_t result = client_exists(client, args[i]);
        if (result == CLIENT_SUCCESS) {
            exists_count++;
        }
    }
    
    char response[32];
    snprintf(response, sizeof(response), "(%d)", exists_count);
    send_response_to_client(client, response);
}

static void handle_keys(client_instance_t* client, const char** args, size_t args_count) {
    if (!client || !args || args_count < 1) {
        return;
    }
    
    struct command_definition_impl* cmd = get_command_secure("KEYS");
    if (!cmd) return;
    
    if (cmd->sec_front != 0x434D4453 || cmd->sec_back != 0x53454355) {
        return;
    }
    
    if (!secure_validate_cmd_id(g_global_cmd_system, cmd->cmd_id)) {
        return;
    }
    
    // TODO: Реализация команды KEYS
    // Пока заглушка
    send_response_to_client(client, "(error: KEYS not implemented)");
}

static void handle_info(client_instance_t* client, const char** args, size_t args_count) {
    if (!client) {
        return;
    }
    
    struct command_definition_impl* cmd = get_command_secure("INFO");
    if (!cmd) return;
    
    if (cmd->sec_front != 0x434D4453 || cmd->sec_back != 0x53454355) {
        return;
    }
    
    if (!secure_validate_cmd_id(g_global_cmd_system, cmd->cmd_id)) {
        return;
    }
    
    char info[1024];
    snprintf(info, sizeof(info),
             "# Client Information\r\n"
             "status:%s\r\n"
             "operations_total:%llu\r\n"
             "operations_failed:%llu\r\n"
             "bytes_sent:%llu\r\n"
             "bytes_received:%llu\r\n"
             "authenticated:%s\r\n",
             client->status == CLIENT_STATUS_CONNECTED ? "connected" : "disconnected",
             (unsigned long long)client->stats.operations_total,
             (unsigned long long)client->stats.operations_failed,
             (unsigned long long)client->stats.bytes_sent,
             (unsigned long long)client->stats.bytes_received,
             "unknown"); // client->authenticated ? "yes" : "no" - закомментировано
    
    send_response_to_client(client, info);
}

static void handle_select(client_instance_t* client, const char** args, size_t args_count) {
    if (!client || !args || args_count < 1) {
        return;
    }
    
    struct command_definition_impl* cmd = get_command_secure("SELECT");
    if (!cmd) return;
    
    if (cmd->sec_front != 0x434D4453 || cmd->sec_back != 0x53454355) {
        return;
    }
    
    if (!secure_validate_cmd_id(g_global_cmd_system, cmd->cmd_id)) {
        return;
    }
    
    const char* db_index_str = args[0];
    char* endptr;
    long db_index = strtol(db_index_str, &endptr, 10);
    
    if (*endptr != '\0' || db_index < 0 || db_index > 15) {
        send_response_to_client(client, "ERR invalid DB index");
        return;
    }
    
    // TODO: Реальная смена базы данных
    // Пока просто подтверждаем
    char response[32];
    snprintf(response, sizeof(response), "OK (selected DB %ld)", db_index);
    send_response_to_client(client, response);
}