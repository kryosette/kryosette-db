CC = gcc
CFLAGS = -O3 -Wall -Wextra -std=c99
TARGET = drs_generator_test

SRCS = drs_generator.c test_exact.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

drs_generator.o: drs_generator.c drs_generator.h
	$(CC) $(CFLAGS) -c $<

test_exact.o: test_exact.c drs_generator.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) $(TARGET)

run: all
	./$(TARGET)

.PHONY: all clean run