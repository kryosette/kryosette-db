# server
gcc -o server main.c server.c /Users/dimaeremin/kryosette-db/kryocache/src/core/server/commands/commands.c /Users/dimaeremin/kryosette-db/kryocache/src/core/server/constants.c /Users/dimaeremin/kryosette-db/third-party/smemset/smemset.c -I/Users/dimaeremin/kryosette-db/kryocache/src/core/server/include -I/Users/dimaeremin/kryosette-db/third-party/smemset/include

# client
gcc -o client main.c client.c /Users/dimaeremin/kryosette-db/kryocache/src/core/client/constants.c -I/Users/dimaeremin/kryosette-db/kryocache/src/core/client/include
./client --verbose ping