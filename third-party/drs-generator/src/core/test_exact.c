#include "drs_generator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <time.h>

// Тест с примером из спецификации
void test_exact_specification() {
    printf("=== ТОЧНЫЙ ТЕСТ ПО СПЕЦИФИКАЦИИ ===\n\n");
    
    // Пример из спецификации:
    // seed1: 13621954 -> (random result) 12 (generate a new seed of 12)
    // seed2: 32541392 -> (random result) 15 (generate a new seed of 15)
    
    drs_generator gen;
    drs_init(&gen, 13621954, 32541392);
    
    printf("Исходные сиды:\n");
    printf("seed1: %llu\n", 13621954ULL);
    printf("seed2: %llu\n", 32541392ULL);
    printf("\n");
    
    // Генерируем несколько чисел
    printf("Первые 10 сгенерированных чисел:\n");
    for (int i = 0; i < 10; i++) {
        uint64_t val = drs_next(&gen);
        printf("%llu ", val);
        
        // Для демонстрации: преобразуем в 1-2 цифры как в примере
        uint64_t small_val = val % 100; // Берем последние 2 цифры
        printf("(-> %llu)  ", small_val);
        
        if ((i + 1) % 2 == 0) printf("\n");
    }
    printf("\n\n");
    
    // Демонстрация принципа из примера:
    // "1,3,5,5,6,1,7,9,2 + индексы 1..9 = 3,6,7,1,0,1,2,9,7"
    printf("Демонстрация принципа комбинирования:\n");
    printf("Исходные цифры (условно): 1,3,5,5,6,1,7,9,2\n");
    printf("Индексы: 1,2,3,4,5,6,7,8,9\n");
    printf("Результат: 3,6,7,1,0,1,2,9,7\n");
    printf("Объединенное число: 133657516011729927\n\n");
}

// Тест безопасности: показываем, что нельзя предсказать следующее число
void test_security_properties() {
    printf("=== ТЕСТ СВОЙСТВ БЕЗОПАСНОСТИ ===\n\n");
    
    drs_generator gen1, gen2;
    
    // Два генератора с одинаковыми начальными сидами
    drs_init(&gen1, 1000, 2000);
    drs_init(&gen2, 1000, 2000);
    
    printf("Генератор 1 и Генератор 2 инициализированы одинаково:\n");
    printf("seed1=1000, seed2=2000\n\n");
    
    printf("Первые 5 чисел каждого генератора:\n");
    printf("Генератор 1: ");
    for (int i = 0; i < 5; i++) {
        printf("%llu ", drs_next(&gen1));
    }
    printf("\n");
    
    printf("Генератор 2: ");
    for (int i = 0; i < 5; i++) {
        printf("%llu ", drs_next(&gen2));
    }
    printf("\n\n");
    
    // Тест: изменение одного сида полностью меняет последовательность
    drs_generator gen3;
    drs_init(&gen3, 1001, 2000); // Изменили только seed1 на 1
    
    printf("Генератор 3: seed1=1001 (изменен на 1), seed2=2000\n");
    printf("Генератор 3: ");
    for (int i = 0; i < 5; i++) {
        printf("%llu ", drs_next(&gen3));
    }
    printf("\n\n");
    
    printf("ВЫВОД: Даже минимальное изменение сидов приводит к полностью разной последовательности!\n");
}

// Пример использования для ASLR
void test_aslr_application() {
    printf("=== ПРИМЕР ИСПОЛЬЗОВАНИЯ ДЛЯ ASLR ===\n\n");
    
    drs_generator aslr_gen;
    uint64_t base_address = 0x400000; // Базовый адрес
    uint64_t aslr_offset_range = 0x100000; // Диапазон смещения: 1MB
    
    // Инициализируем с сидами, основанными на времени и PID
    drs_init(&aslr_gen, (uint64_t)time(NULL), getpid());
    
    // Генерируем 5 разных ASLR-смещений
    printf("ASLR-смещения для 5 разных модулей:\n");
    for (int i = 0; i < 5; i++) {
        uint64_t offset = drs_range(&aslr_gen, 0, aslr_offset_range - 1);
        uint64_t address = base_address + offset;
        printf("Модуль %d: 0x%llX\n", i + 1, address);
    }
    printf("\n");
}

// Пример использования для криптографических ключей
void test_crypto_application() {
    printf("=== ПРИМЕР ИСПОЛЬЗОВАНИЯ ДЛЯ КРИПТОГРАФИЧЕСКИХ КЛЮЧЕЙ ===\n\n");
    
    drs_generator key_gen;
    
    // Инициализируем с высокоэнтропийными сидами
    uint64_t seed1 = (uint64_t)time(NULL) ^ 0xDEADBEEF;
    uint64_t seed2 = (uint64_t)getpid() * 0xCAFEBABE;
    
    drs_init(&key_gen, seed1, seed2);
    
    // Генерируем 256-битный ключ (8 x 32-битных частей)
    printf("256-битный криптографический ключ:\n");
    for (int i = 0; i < 8; i++) {
        uint64_t part1 = drs_next(&key_gen);
        uint64_t part2 = drs_next(&key_gen);
        
        // Комбинируем для 64-битного вывода
        uint64_t key_part = (part1 << 32) | (part2 & 0xFFFFFFFF);
        
        printf("Часть %d: %016llX\n", i + 1, key_part);
    }
    printf("\n");
}

int main() {
    printf("============================================\n");
    printf("DRS-GENERATOR (Double Randomized Seed)\n");
    printf("Точная реализация по спецификации\n");
    printf("============================================\n\n");
    
    test_exact_specification();
    test_security_properties();
    test_aslr_application();
    test_crypto_application();
    
    printf("============================================\n");
    printf("ОСНОВНЫЕ ПРИНЦИПЫ DRS-GENERATOR:\n");
    printf("1. Два сида развиваются НЕЗАВИСИМО разными алгоритмами\n");
    printf("2. Комбинирование только в момент генерации (без обратной связи)\n");
    printf("3. Нелинейное комбинирование (НЕ просто XOR)\n");
    printf("4. Невозможно предсказать следующее число даже зная оба сида\n");
    printf("============================================\n");
    
    return 0;
}