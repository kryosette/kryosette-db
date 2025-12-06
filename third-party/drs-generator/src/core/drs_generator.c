#include "drs_generator.h"

// Первый LCG-генератор для seed1 (основная последовательность)
static uint64_t lcg1_next(uint64_t* state) {
    // Параметры LCG: a = 1103515245, c = 12345, m = 2^31
    *state = (1103515245ULL * *state + 12345) % 2147483648ULL;
    return *state;
}

// Второй LCG-генератор для seed2 (смещения и трансформации)
// Используем другие параметры для независимого развития
static uint64_t lcg2_next(uint64_t* state) {
    // Параметры LCG: a = 1664525, c = 1013904223, m = 2^32
    *state = (1664525ULL * *state + 1013904223) % 4294967296ULL;
    return *state;
}

// Нелинейная функция смешивания (аналог примера в спецификации)
static uint64_t nonlinearly_combine(uint64_t a, uint64_t b, uint64_t index) {
    // Преобразуем числа в цифры для обработки как в примере
    uint64_t digits_a[20], digits_b[20];
    int count_a = 0, count_b = 0;
    
    // Извлекаем цифры из a
    uint64_t temp = a;
    while (temp > 0 && count_a < 20) {
        digits_a[count_a++] = temp % 10;
        temp /= 10;
    }
    
    // Извлекаем цифры из b
    temp = b;
    while (temp > 0 && count_b < 20) {
        digits_b[count_b++] = temp % 10;
        temp /= 10;
    }
    
    // Создаем результат как в примере: цифры + индекс (не XOR!)
    uint64_t result = 0;
    
    // Обрабатываем максимальное количество цифр
    int max_count = (count_a > count_b) ? count_a : count_b;
    if (max_count == 0) max_count = 1; // Минимум одна цифра
    
    for (int i = 0; i < max_count; i++) {
        uint64_t digit_a = (i < count_a) ? digits_a[i] : 0;
        uint64_t digit_b = (i < count_b) ? digits_b[i] : 0;
        
        // НЕЛИНЕЙНОЕ КОМБИНИРОВАНИЕ как в спецификации:
        // В примере: 1,3,5,5,6,1,7,9,2 + индексы 1..9 = 3,6,7,1,0,1,2,9,7
        uint64_t combined = (digit_a + digit_b + (index % 10)) % 10;
        result = result * 10 + combined;
    }
    
    return result;
}

void drs_init(drs_generator* gen, uint64_t seed1, uint64_t seed2) {
    gen->seed1 = seed1;
    gen->seed2 = seed2;
    gen->counter = 0;
    
    // "Прогрев" генератора - несколько итераций без вывода
    for (int i = 0; i < 10; i++) {
        drs_next(gen);
    }
}

// Получение следующего случайного числа
uint64_t drs_next(drs_generator* gen) {
    // Увеличиваем счетчик (аналог индекса в примере)
    gen->counter++;
    
    // РАЗВИТИЕ СИДОВ НЕЗАВИСИМО (ключевая особенность!)
    // seed1 развивается по первому алгоритму
    uint64_t out1 = lcg1_next(&gen->seed1);
    
    // seed2 развивается по второму алгоритму - РАЗНЫЙ алгоритм!
    uint64_t out2 = lcg2_next(&gen->seed2);
    
    // МГНОВЕННОЕ КОМБИНИРОВАНИЕ ТОЛЬКО В МОМЕНТ ГЕНЕРАЦИИ
    // БЕЗ ОБРАТНОЙ СВЯЗИ между сидами!
    uint64_t result = nonlinearly_combine(out1, out2, gen->counter);
    
    return result;
}

// Генерация числа в диапазоне [min, max]
uint64_t drs_range(drs_generator* gen, uint64_t min, uint64_t max) {
    uint64_t random_value = drs_next(gen);
    
    if (min > max) {
        // Меняем местами если min > max
        uint64_t temp = min;
        min = max;
        max = temp;
    }
    
    uint64_t range = max - min + 1;
    return min + (random_value % range);
}