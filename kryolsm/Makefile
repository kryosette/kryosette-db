CC=gcc
CFLAGS=-Wall -Wextra -Iinclude
LDFLAGS=
SOURCES=src/lsm.c src/sstable.c src/main.c
BINARY=lsm_db

all:
    $(CC) $(CFLAGS) $(SOURCES) -o $(BINARY) $(LDFLAGS)

clean:
    rm -f $(BINARY) data/*.sst