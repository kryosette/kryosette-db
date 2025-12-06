#ifndef DRS_GENERATOR_H
#define DRS_GENERATOR_H

#include <stdint.h>

// Структура DRS-генератора
typedef struct {
    uint64_t seed1;      // Первый сид (основная последовательность)
    uint64_t seed2;      // Второй сид (смещения и трансформации)
    uint64_t counter;    // Счетчик вызовов
} drs_generator;

// Инициализация генератора
void drs_init(drs_generator* gen, uint64_t seed1, uint64_t seed2);

// Получение следующего случайного числа
uint64_t drs_next(drs_generator* gen);

// Генерация числа в диапазоне [min, max]
uint64_t drs_range(drs_generator* gen, uint64_t min, uint64_t max);

#endif