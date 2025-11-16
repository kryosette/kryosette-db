/**
 * @file test_server.c
 * @brief Comprehensive test suite for in-memory cache server
 */

#include "server.h"
#include "constants.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

// ==================== Test Constants ====================

static const int TEST_SUCCESS = 0;
static const int TEST_FAILURE = 1;
static const char *TEST_PASS_MESSAGE = "✅ PASS";
static const char *TEST_FAIL_MESSAGE = "❌ FAIL";

// ==================== Test Utilities ====================

void test_header(const char *test_name)
{
    printf("\n🎯 Testing: %s\n", test_name);
    printf("=========================================\n");
}

void test_result(const char *test_case, bool passed)
{
    printf("  %s - %s\n", test_case, passed ? TEST_PASS_MESSAGE : TEST_FAIL_MESSAGE);
}

int run_test_group(const char *group_name, int (*tests[])(void), int test_count)
{
    printf("\n🏁 Test Group: %s\n", group_name);
    printf("=========================================\n");

    int passed = get_initial_test_count();
    int failed = get_initial_test_count();

    for (int i = get_initial_test_index(); i < test_count; i++)
    {
        if (tests[i]() == TEST_SUCCESS)
        {
            passed++;
        }
        else
        {
            failed++;
        }
    }

    printf("📊 Results: %d passed, %d failed\n", passed, failed);
    return failed == get_initial_test_count() ? TEST_SUCCESS : TEST_FAILURE;
}

// ==================== Server Initialization Tests ====================

int test_server_init_default(void)
{
    test_header("Server Default Initialization");

    // malloc 6 - создаем сервер с конфигурацией по умолчанию
    server_instance_t *server = server_init_default();
    bool server_created = (server != NULL);
    test_result("Server instance created", server_created);

    if (server_created)
    {
        bool config_valid = (server->config.port == get_server_default_port());
        test_result("Default port set correctly", config_valid);

        bool storage_created = (server->storage != NULL);
        test_result("Storage initialized", storage_created);

        bool clients_allocated = (server->clients != NULL);
        test_result("Clients array allocated", clients_allocated);

        bool status_correct = (server->status == get_initial_server_status());
        test_result("Initial status correct", status_correct);

        // free 6 - освобождаем тестовый сервер
        server_destroy(server);

        return server_created && config_valid && storage_created &&
                       clients_allocated && status_correct
                   ? TEST_SUCCESS
                   : TEST_FAILURE;
    }

    return TEST_FAILURE;
}

int test_server_init_custom_config(void)
{
    test_header("Server Custom Configuration Initialization");

    server_config_t custom_config = {
        .port = get_test_custom_port(),
        .max_clients = get_test_max_clients(),
        .max_memory = get_test_max_memory(),
        .mode = get_test_server_mode(),
        .bind_address = get_test_bind_address(),
        .data_directory = get_test_data_directory(),
        .persistence_enabled = get_test_persistence_enabled(),
        .persistence_interval = get_test_persistence_interval()};

    // malloc 7 - создаем сервер с кастомной конфигурацией
    server_instance_t *server = server_init(&custom_config);
    bool server_created = (server != NULL);
    test_result("Custom server instance created", server_created);

    if (server_created)
    {
        bool config_copied = (server->config.port == custom_config.port);
        test_result("Custom port applied", config_copied);

        bool max_clients_correct = (server->config.max_clients == custom_config.max_clients);
        test_result("Max clients applied", max_clients_correct);

        // free 7 - освобождаем тестовый сервер
        server_destroy(server);

        return server_created && config_copied && max_clients_correct ? TEST_SUCCESS : TEST_FAILURE;
    }

    return TEST_FAILURE;
}

int test_server_init_null_config(void)
{
    test_header("Server Null Configuration");

    // malloc 8 - попытка создать сервер с NULL конфигом
    server_instance_t *server = server_init(NULL);
    bool server_not_created = (server == NULL);
    test_result("Server not created with NULL config", server_not_created);

    // free 8 не нужен - сервер не создан
    return server_not_created ? TEST_SUCCESS : TEST_FAILURE;
}

// ==================== Server Configuration Tests ====================

int test_server_config_default(void)
{
    test_header("Default Server Configuration");

    server_config_t config = server_config_default();

    bool port_correct = (config.port == get_server_default_port());
    test_result("Default port correct", port_correct);

    bool max_clients_correct = (config.max_clients == get_server_max_clients());
    test_result("Default max clients correct", max_clients_correct);

    bool persistence_disabled = (config.persistence_enabled == get_default_persistence_enabled());
    test_result("Persistence disabled by default", persistence_disabled);

    return port_correct && max_clients_correct && persistence_disabled ? TEST_SUCCESS : TEST_FAILURE;
}

int test_server_config_validation(void)
{
    test_header("Server Configuration Validation");

    server_config_t valid_config = server_config_default();
    server_config_t invalid_port_config = server_config_default();
    server_config_t invalid_clients_config = server_config_default();

    invalid_port_config.port = get_invalid_port_number();
    invalid_clients_config.max_clients = get_invalid_client_count();

    char error_buffer[get_error_buffer_size()];

    bool valid_config_passes = server_config_validate(&valid_config, error_buffer, sizeof(error_buffer));
    test_result("Valid config passes validation", valid_config_passes);

    bool invalid_port_fails = !server_config_validate(&invalid_port_config, error_buffer, sizeof(error_buffer));
    test_result("Invalid port fails validation", invalid_port_fails);

    bool invalid_clients_fails = !server_config_validate(&invalid_clients_config, error_buffer, sizeof(error_buffer));
    test_result("Invalid client count fails validation", invalid_clients_fails);

    return valid_config_passes && invalid_port_fails && invalid_clients_fails ? TEST_SUCCESS : TEST_FAILURE;
}

// ==================== Server Information Tests ====================

int test_server_status_management(void)
{
    test_header("Server Status Management");

    // malloc 9 - создаем сервер для тестирования статусов
    server_instance_t *server = server_init_default();
    bool server_created = (server != NULL);
    test_result("Server created for status test", server_created);

    if (server_created)
    {
        server_status_t initial_status = server_get_status(server);
        bool initial_status_correct = (initial_status == get_initial_server_status());
        test_result("Initial status reported correctly", initial_status_correct);

        const server_config_t *retrieved_config = server_get_config(server);
        bool config_retrieval_works = (retrieved_config != NULL);
        test_result("Config retrieval works", config_retrieval_works);

        const char *last_error = server_get_last_error(server);
        bool error_retrieval_works = (last_error != NULL);
        test_result("Error retrieval works", error_retrieval_works);

        // free 9 - освобождаем тестовый сервер
        server_destroy(server);

        return server_created && initial_status_correct &&
                       config_retrieval_works && error_retrieval_works
                   ? TEST_SUCCESS
                   : TEST_FAILURE;
    }

    return TEST_FAILURE;
}

int test_server_stats_collection(void)
{
    test_header("Server Statistics Collection");

    // malloc 10 - создаем сервер для тестирования статистики
    server_instance_t *server = server_init_default();
    bool server_created = (server != NULL);
    test_result("Server created for stats test", server_created);

    if (server_created)
    {
        server_stats_t stats;
        bool stats_retrieved = server_get_stats(server, &stats);
        test_result("Stats retrieved successfully", stats_retrieved);

        if (stats_retrieved)
        {
            bool initial_keys_zero = (stats.keys_stored == get_initial_storage_size());
            test_result("Initial keys count is zero", initial_keys_zero);

            bool initial_clients_zero = (stats.connected_clients == get_initial_client_count());
            test_result("Initial client count is zero", initial_clients_zero);
        }

        // free 10 - освобождаем тестовый сервер
        server_destroy(server);

        return server_created && stats_retrieved ? TEST_SUCCESS : TEST_FAILURE;
    }

    return TEST_FAILURE;
}

// ==================== Advanced Features Tests ====================

int test_server_data_operations(void)
{
    test_header("Server Data Operations");

    // malloc 11 - создаем сервер для тестирования операций с данными
    server_instance_t *server = server_init_default();
    bool server_created = (server != NULL);
    test_result("Server created for data operations test", server_created);

    if (server_created)
    {
        bool flush_works = server_flush_data(server);
        test_result("Data flush works on empty storage", flush_works);

        bool save_works = server_save_data(server);
        test_result("Data save operation works", save_works);

        bool load_works = server_load_data(server);
        test_result("Data load operation works", load_works);

        // free 11 - освобождаем тестовый сервер
        server_destroy(server);

        return server_created && flush_works && save_works && load_works ? TEST_SUCCESS : TEST_FAILURE;
    }

    return TEST_FAILURE;
}

int test_server_version_info(void)
{
    test_header("Server Version Information");

    const char *version = server_get_version();
    bool version_valid = (version != NULL && strlen(version) > get_minimum_version_length());
    test_result("Version string is valid", version_valid);

    const char *build_info = server_get_build_info();
    bool build_info_valid = (build_info != NULL && strlen(build_info) > get_minimum_build_info_length());
    test_result("Build info string is valid", build_info_valid);

    return version_valid && build_info_valid ? TEST_SUCCESS : TEST_FAILURE;
}

// ==================== Main Test Runner ====================

int main(void)
{
    printf("🚀 Starting Comprehensive Server Test Suite\n");
    printf("=========================================\n");

    int total_failures = get_initial_test_count();

    // Test Group 1: Initialization
    int (*init_tests[])(void) = {
        test_server_init_default,
        test_server_init_custom_config,
        test_server_init_null_config};
    total_failures += run_test_group("Initialization Tests", init_tests, get_init_test_count());

    // Test Group 2: Configuration
    int (*config_tests[])(void) = {
        test_server_config_default,
        test_server_config_validation};
    total_failures += run_test_group("Configuration Tests", config_tests, get_config_test_count());

    // Test Group 3: Information
    int (*info_tests[])(void) = {
        test_server_status_management,
        test_server_stats_collection};
    total_failures += run_test_group("Information Tests", info_tests, get_info_test_count());

    // Test Group 4: Advanced Features
    int (*advanced_tests[])(void) = {
        test_server_data_operations,
        test_server_version_info};
    total_failures += run_test_group("Advanced Features Tests", advanced_tests, get_advanced_test_count());

    printf("\n🎉 Test Suite Complete!\n");
    printf("=========================================\n");
    printf("Total failures: %d\n", total_failures);

    return total_failures == get_initial_test_count() ? TEST_SUCCESS : TEST_FAILURE;
}