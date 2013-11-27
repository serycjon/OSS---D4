SRC=./src
BIN=./bin

all: main sample_udp man_sample parser

main: $(SRC)/main.c
	gcc $(SRC)/main.c -o $(BIN)/main

sample_udp: $(SRC)/sample_udp_server.c $(SRC)/sample_udp_client.c
	gcc $(SRC)/sample_udp_server.c -o $(BIN)/server
	gcc $(SRC)/sample_udp_client.c -o $(BIN)/client

man_sample: $(SRC)/man_server.c $(SRC)/man_client.c
	gcc $(SRC)/man_server.c -o $(BIN)/man_server
	gcc $(SRC)/man_client.c -o $(BIN)/man_client

parser: $(SRC)/route_cfg_parser.c $(SRC)/route_cfg_parser.h $(SRC)/test_cfg_parser.c
	gcc $(SRC)/route_cfg_parser.c $(SRC)/test_cfg_parser.c -o $(BIN)/parser

clean: rm -rf *.o
