SRC=./src
BIN=./bin

all: main sample_udp man_sample

main: $(SRC)/main.c
	gcc $(SRC)/main.c -o $(BIN)/main

sample_udp: $(SRC)/sample_udp_server.c $(SRC)/sample_udp_client.c
	gcc $(SRC)/sample_udp_server.c -o $(BIN)/server
	gcc $(SRC)/sample_udp_client.c -o $(BIN)/client

man_sample: $(SRC)/man_server.c $(SRC)/man_client.c
	gcc $(SRC)/man_server.c -o $(BIN)/man_server
	gcc $(SRC)/man_client.c -o $(BIN)/man_client

clean: rm -rf *.o
