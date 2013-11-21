SRC=./src
BIN=./bin

all: main sample_udp

main: $(SRC)/main.c
	gcc $(SRC)/main.c -o $(BIN)/main

sample_udp: $(SRC)/sample_udp_server.c $(SRC)/sample_udp_client.c
	gcc $(SRC)/sample_udp_server.c -o $(BIN)/server
	gcc $(SRC)/sample_udp_client.c -o $(BIN)/client

clean: rm -rf *.o
