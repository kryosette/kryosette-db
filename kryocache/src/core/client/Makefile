# Makefile для тестов безопасности
CC = gcc
CFLAGS = -Wall -Wextra -Werror -O2 -g -D_POSIX_C_SOURCE=200112L
LDFLAGS = -lpthread

# Исходные файлы
SRC_CLIENT = ../src/core/client/client.c
SRC_WHITELIST = ../white_list/client/white_list_client.c
SRC_DRS = ../third-party/drs-generator/src/core/drs_generator.c
SRC_TESTS = security_stress_test.c

# Объектные файлы
OBJ_CLIENT = client.o
OBJ_WHITELIST = white_list_client.o
OBJ_DRS = drs_generator.o
OBJ_TESTS = security_stress_test.o

# Заголовочные файлы
INCLUDES = -I../src/core/client/include \
           -I../white_list/client \
           -I../third-party/drs-generator/src/core \
           -I.

# Цели
TARGET = security_test

all: $(TARGET)

$(TARGET): $(OBJ_CLIENT) $(OBJ_WHITELIST) $(OBJ_DRS) $(OBJ_TESTS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Специальные правила для файлов из других директорий
client.o: $(SRC_CLIENT)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

white_list_client.o: $(SRC_WHITELIST)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

drs_generator.o: $(SRC_DRS)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Тесты
test: $(TARGET)
	./$(TARGET)

# Fuzzing тест
fuzz_test: $(TARGET)
	@echo "Запуск fuzzing тестов..."
	@for i in `seq 1 100`; do \
		echo "Итерация $$i/100"; \
		./$(TARGET) > /dev/null 2>&1; \
		if [ $$? -ne 0 ]; then \
			echo "Найден баг на итерации $$i!"; \
			exit 1; \
		fi; \
	done
	@echo "Fuzzing тесты пройдены успешно!"

# Тесты на утечки памяти
valgrind_test: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET)

# Очистка
clean:
	rm -f $(TARGET) *.o

.PHONY: all test fuzz_test valgrind_test clean