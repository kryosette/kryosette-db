#include "white_list_client.h"
#include "/Users/dimaeremin/kryosette-db/third-party/drs-generator/src/core/drs_generator.h"
#include "/Users/dimaeremin/kryosette-db/third-party/smemset/include/smemset.h"

#include <stdlib.h>   
#include <string.h>  
#include <time.h>

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
    } cmd_templates[] = {
        {"GET",    1, 1, validate_key,   handle_get},
        {"SET",    2, 2, validate_kv,    handle_set},
        {"DEL",    1, 5, validate_keys,  handle_delete},
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
            if (strncmp(CMD_NAMES[j], cmd_templates[i].name, strlen(cmd_templates[i].name)) == 0) { 
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
}

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
    {0xDEADBEEF12345678, 0xCAFEBABE87654321}, // GET
    {0xBEEFDEAD56781234, 0xFACEFACE43218765}, // SET
    {0xFEEDFACE87654321, 0xDECAFBAD12345678}, // DELETE
    {0xCAFEDEAD43218765, 0xBEEFFACE56781234}, // EXISTS
    {0xDEADC0DE56781234, 0xFACEB00C43218765}, // KEYS
    {0xBADCAFE87654321, 0xDEADC0DE12345678},  // PING
    {0xC0FFEE12345678, 0xFEABA687654321},     // INFO
    {0xBABEFACE43218765, 0xDEADD00D56781234}, // QUIT
    {0xFACEFEED87654321, 0xCAFEB0BA12345678}, // AUTH
    {0xDEADFACE56781234, 0xFEEDBEEF43218765}, // SELECT
};

static const char *CMD_NAMES[10] = {
    "GET", "SET", "DELETE", "EXISTS", "KEYS",
    "PING", "INFO", "QUIT", "AUTH", "SELECT"
};

struct enum_system_impl
{
    drs_generator gen;
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

static void safe_to_upper_string(char *str, size_t max_len) {
    if (!str || max_len <= 1) return;

    size_t i = 0;
    for (i = 0; i < max_len - 1 && str[i] != '\0'; i++) {
        unsigned char c = str[i];
        if (c - 'a' < 26) {
            str[i] = c ^ 0x20;
        }
    }

    if (i < max_len) {
        str[i] = '\0';
    } else if (max_len > 0) {
        str[max_len - 1] = '\0';
    }
}

static secure_cmd_id_t generate_secure_id(drs_generator *gen, int cmd_index) {
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
        drs_generator cmd_gen;
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
            uint64_t check = cmd_id ^ enum_sys->validation_keys[i];
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

    smemset(enum_sys->generated_ids, 0, sizeof(enum_sys->generated_ids));
    smemset(enum_sys->validation_keys, 0, sizeof(enum_sys->validation_keys));

    free(enum_sys);
}

