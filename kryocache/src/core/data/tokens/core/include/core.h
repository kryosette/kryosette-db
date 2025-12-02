#pragma once

typedef struct 
{
    char* id;
    char* user_id;
    char* username;
    char** authorities;
    size_t authorities_count;
    char* device_hash;
    time_t issued_at;
    time_t expires_at;
    char* client_ip;
} token_data_t;
