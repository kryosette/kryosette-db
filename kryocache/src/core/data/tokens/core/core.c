#include "/Users/dimaeremin/kryosette-db/kryocache/src/core/data/tokens/core/include/core.h"

token_data_t *token_data_create(const char* id, const char* user_id, 
                            const char* username, const char** authorities, 
                            size_t auth_count, const char* device_hash,
                            time_t issued_at, time_t expires_at) 
{
    token_data_t *token_data = (token_data_create*)calloc(sizeof(token_data_create));
    // temp \ warning
    if (id == NULL) return NULL;

    token_data->id = 
}