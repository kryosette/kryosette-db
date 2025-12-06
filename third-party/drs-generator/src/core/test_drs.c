#include "drs_generator.h"
#include <stdio.h>
#include <time.h>

// Тестирование генератора
void test_drs_generator() {
    drs_generator gen;
    
    // Инициализация временем и PID для уникальности
    uint64_t seed1 = (uint64_t)time(NULL);
    uint64_t seed2 = seed1 * 0x123456789ABCDEFULL;
    
    drs_init(&gen, seed1, seed2);
    
    printf("DRS-Generator Test:\n");
    printf("Seed1: %llu, Seed2: %llu\n\n", seed1, seed2);
    
    printf("10 random numbers [0, 99]:\n");
    for (int i = 0; i < 10; i++) {
        printf("%llu ", drs_range(&gen, 0, 99));
    }
    printf("\n\n");
    
    printf("Random bytes (hex):\n");
    uint8_t bytes[16];
    drs_bytes(&gen, bytes, sizeof(bytes));
    
    for (int i = 0; i < 16; i++) {
        printf("%02X ", bytes[i]);
    }
    printf("\n");
    
    // Тест на равномерность распределения
    printf("\nDistribution test (0-9, 10000 samples):\n");
    int counts[10] = {0};
    
    for (int i = 0; i < 10000; i++) {
        uint64_t val = drs_range(&gen, 0, 9);
        counts[val]++;
    }
    
    for (int i = 0; i < 10; i++) {
        printf("%d: %d (%.1f%%)\n", i, counts[i], counts[i] / 100.0);
    }
}

// Пример использования для безопасных применений
int main() {
    printf("=== Double Randomized Seed Generator ===\n\n");
    
    // Пример 1: Генерация сессионного ключа
    {
        drs_generator session_gen;
        uint64_t session_seed1 = (uint64_t)time(NULL) ^ 0xDEADBEEF;
        uint64_t session_seed2 = session_seed1 * 0xCAFEBABE;
        
        drs_init(&session_gen, session_seed1, session_seed2);
        
        uint8_t session_key[32];
        drs_bytes(&session_gen, session_key, sizeof(session_key));
        
        printf("Session Key (hex):\n");
        for (int i = 0; i < 32; i++) {
            printf("%02X", session_key[i]);
        }
        printf("\n\n");
    }
    
    // Пример 2: ASLR-подобное смещение
    {
        drs_generator aslr_gen;
        uint64_t aslr_base = 0x100000;
        uint64_t aslr_range = 0x10000;
        
        // Нужно инициализировать генератор для ASLR
        uint64_t aslr_seed1 = (uint64_t)time(NULL) ^ 0xABCD1234;
        uint64_t aslr_seed2 = aslr_seed1 * 0x987654321ULL;
        drs_init(&aslr_gen, aslr_seed1, aslr_seed2);
        
        uint64_t offset = drs_range(&aslr_gen, 0, aslr_range - 1);
        uint64_t address = aslr_base + offset;
        
        printf("ASLR Address: 0x%llX\n\n", address);
    }
    
    // Запуск тестов
    test_drs_generator();
    
    return 0;
}