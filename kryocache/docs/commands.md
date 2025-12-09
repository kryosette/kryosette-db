# server
gcc -o server main.c server.c /Users/dimaeremin/kryosette-db/kryocache/src/core/server/commands/commands.c /Users/dimaeremin/kryosette-db/kryocache/src/core/server/constants.c /Users/dimaeremin/kryosette-db/third-party/smemset/smemset.c -I/Users/dimaeremin/kryosette-db/kryocache/src/core/server/include -I/Users/dimaeremin/kryosette-db/third-party/smemset/include

# client
gcc -o client main.c client.c /Users/dimaeremin/kryosette-db/kryocache/src/core/client/constants.c -I/Users/dimaeremin/kryosette-db/kryocache/src/core/client/include
./client --verbose ping


gcc -o client \
  main.c \
  client.c \
  /Users/dimaeremin/kryosette-db/kryocache/src/core/client/constants.c \
  /Users/dimaeremin/kryosette-db/kryocache/white_list/client/white_list_client.c \
  /Users/dimaeremin/kryosette-db/kryocache/white_list/client/command_stubs.c \
  -I/Users/dimaeremin/kryosette-db/kryocache/src/core/client/include \
  -I/Users/dimaeremin/kryosette-db/kryocache/white_list/client \
  -w

gcc -o client \
  main.c \
  client.c \
  /Users/dimaeremin/kryosette-db/kryocache/src/core/client/constants.c \
  /Users/dimaeremin/kryosette-db/kryocache/white_list/client/white_list_client.c \
  /Users/dimaeremin/kryosette-db/kryocache/white_list/client/command_stubs.c \
  /Users/dimaeremin/kryosette-db/third-party/drs-generator/src/core/drs_generator.o \
  -I/Users/dimaeremin/kryosette-db/kryocache/src/core/client/include \
  -I/Users/dimaeremin/kryosette-db/kryocache/white_list/client \
  -I/Users/dimaeremin/kryosette-db/third-party/drs-generator/src/core \
  -I/Users/dimaeremin/kryosette-db/third-party/smemset/include \
  -w