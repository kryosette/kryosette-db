#include "white_list_client.h"
#include "/Users/dimaeremin/kryosette-db/third-party/drs-generator/src/core/drs_generator.h"

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
    {0xBADCAFE87654321, 0xDEADCODE12345678},  // PING
    {0xCOFFEE12345678, 0xTEABAG87654321},     // INFO
    {0xBABEFACE43218765, 0xDEADD00D56781234}, // QUIT
    {0xFACEFEED87654321, 0xCAFEB0BA12345678}, // AUTH
    {0xDEADFACE56781234, 0xFEEDBEEF43218765}, // SELECT
};

static const char* CMD_NAMES[] = {
    "GET", "SET", "DELETE", "EXISTS", "KEYS",
    "PING", "INFO", "QUIT", "AUTH", "SELECT"
};

struct enum_system
{
    drs_generator gen;
    secure_cmd_id_t generated_ids[10];
    uint64_t validation_keys[10];
    time_t init_time;
    uint32_t;
};


struct command_definition {
    const char *cmd_name;
    white_list cmd_id;
    size_t min_args;
    size_t max_args;
    bool (*validator)(const char **args, size_t args_count);
    void (*handler)(client_instance_t *client, const char **args, size_t args_count);
};


static const command_definition valid_commands[] = {
    {"GET",    CMD_GET,    1, 1, validate_key,   handle_get},
    {"SET",    CMD_SET,    2, 2, validate_kv,    handle_set},
    {"DEL",    CMD_DELETE, 1, 5, validate_keys,  handle_delete},
    {"PING",   CMD_PING,   0, 0, NULL,           handle_ping},
    {"QUIT",   CMD_QUIT,   0, 0, NULL,           handle_quit},
    {"AUTH",   CMD_AUTH,   1, 1, validate_auth,  handle_auth},
};

static secure_cmd_id_t generate_secure_id(drs_generator *gen) {
    uint64_t base = drs_next(gen);

    /*
    Why 0x9e3779b9?

    This is an approximation of the golden ratio *2^32
    Widely used in hash functions (e.g. murmurhash)
    Good scattering properties (avalanche effect)
    */
    uint64_t transformed = base ^ (cmd_index * 0x9e3779b9);

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
    uint64_t magic = drs_range(gen, 0x10000000, 0xFFFFFFFF);
    transformed ^= magic;

    return transformed;
}

enum_system_t enum_system_init(uint64_t seed) {
    struct enum_system *sys = calloc(1, sizeof(*sys));
    if (!sys) return NULL;

    uint64_t seed1 = master_seed ^ 0xDEADBEEFCAFEBABE;
    /*
    '~' - inversion
    */
    uint64_t seed2 = ~master_seed ^ 0xBEEFDEADFACEFACE;
    drs_init(&sys->gen, seed1, seed2); 

    for (int i = 0; i < 10; i++) {
        drs_generator cmd_gen;
        drs_init(&cmd_gen, CMD_SEEDS[i][0], CMD_SEEDS[i][1]);

        sys->generated_ids[i] = generate_secure_id(&cmd_gen, i);
    
        sys->validation_keys[i] = drs_range(&cmd_gen, 0x1000, 0xFFFF);
        sys->validation_keys[i] ^= sys->generated_ids[i];
        sys->validation_keys[i] = (sys->validation_keys[i] << 16) | 
                              (sys->validation_keys[i] >> 48);
    }

    return (enum_system_t) sys;
}

static void safe_to_upper_string(char *str, size_t max_len) {
    for (size_t i = 0; i < max_len; i++) {

    }
}