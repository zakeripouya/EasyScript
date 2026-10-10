CC = gcc
CFLAGS = -Wall -Wextra -I./src

SRC = src/lexer.c src/parser.c src/codegen.c
OBJ = $(SRC:.c=.o)

all: easyscript

easyscript: $(OBJ) main.o
	$(CC) $(CFLAGS) -o easyscript $(OBJ) main.o

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

src/lexer.o: src/lexer.c src/lexer.h
	$(CC) $(CFLAGS) -c src/lexer.c -o src/lexer.o

src/parser.o: src/parser.c src/parser.h src/lexer.h
	$(CC) $(CFLAGS) -c src/parser.c -o src/parser.o

src/codegen.o: src/codegen.c src/codegen.h src/parser.h
	$(CC) $(CFLAGS) -c src/codegen.c -o src/codegen.o

clean:
	rm -f $(OBJ) main.o easyscript output.c output_program

.PHONY: clean
