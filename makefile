GCC_OPTIONS=-Wall

SRC=./src
BIN=./bin

all: main sample_udp man_sample parser

main: $(SRC)/main.c
	gcc $(GCC_OPTIONS) $(SRC)/main.c $(SRC)/routing_table.c $(SRC)/route_cfg_parser.c -o $(BIN)/main

sample_udp: $(SRC)/samples/sample_udp_server.c $(SRC)/samples/sample_udp_client.c
	gcc $(GCC_OPTIONS) $(SRC)/samples/sample_udp_server.c -o $(BIN)/server
	gcc $(GCC_OPTIONS) $(SRC)/samples/sample_udp_client.c -o $(BIN)/client

man_sample: $(SRC)/samples/man_server.c $(SRC)/samples/man_client.c
	gcc $(GCC_OPTIONS) $(SRC)/samples/man_server.c -o $(BIN)/man_server
	gcc $(GCC_OPTIONS) $(SRC)/samples/man_client.c -o $(BIN)/man_client

parser: $(SRC)/route_cfg_parser.c $(SRC)/route_cfg_parser.h $(SRC)/test_cfg_parser.c
	gcc $(GCC_OPTIONS) $(SRC)/route_cfg_parser.c $(SRC)/test_cfg_parser.c -o $(BIN)/parser

clean: rm -rf *.o
