CC = gcc
CFLAGS = -Wall -Wextra -I./src

SRC = src/arena.c src/lexer.c src/parser.c src/codegen.c
OBJ = $(SRC:.c=.o)

all: easyscript

easyscript: $(OBJ) main.o
	$(CC) $(CFLAGS) -o easyscript $(OBJ) main.o

main.o: main.c src/arena.h src/lexer.h src/parser.h src/codegen.h
	$(CC) $(CFLAGS) -c main.c

src/arena.o: src/arena.c src/arena.h
	$(CC) $(CFLAGS) -c src/arena.c -o src/arena.o

src/lexer.o: src/lexer.c src/lexer.h
	$(CC) $(CFLAGS) -c src/lexer.c -o src/lexer.o

src/parser.o: src/parser.c src/parser.h src/lexer.h
	$(CC) $(CFLAGS) -c src/parser.c -o src/parser.o

src/codegen.o: src/codegen.c src/codegen.h src/parser.h
	$(CC) $(CFLAGS) -c src/codegen.c -o src/codegen.o

test: easyscript
	sh tests/run.sh

clean:
	rm -f $(OBJ) main.o easyscript

.PHONY: all clean test
