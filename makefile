SRC=./src
BIN=./bin

all: main

main: $(SRC)/main.c
	gcc $(SRC)/main.c -o $(BIN)/main

clean: rm -rf *.o
