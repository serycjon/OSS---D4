GCC_OPTIONS=-Wall -g

SRC=./src
BIN=./bin

all: main

main: $(SRC)/main.c $(SRC)/routing_table.c $(SRC)/topology.c $(SRC)/route_cfg_parser.c $(SRC)/dynamic_routing.c
	gcc $(GCC_OPTIONS) $(SRC)/main.c $(SRC)/routing_table.c $(SRC)/topology.c $(SRC)/dynamic_routing.c $(SRC)/route_cfg_parser.c -o $(BIN)/main -lpthread

sample_udp: $(SRC)/samples/sample_udp_server.c $(SRC)/samples/sample_udp_client.c
	gcc $(GCC_OPTIONS) $(SRC)/samples/sample_udp_server.c -o $(BIN)/server -lpthread
	gcc $(GCC_OPTIONS) $(SRC)/samples/sample_udp_client.c -o $(BIN)/client -lpthread

man_sample: $(SRC)/samples/man_server.c $(SRC)/samples/man_client.c
	gcc $(GCC_OPTIONS) $(SRC)/samples/man_server.c -o $(BIN)/man_server -lpthread
	gcc $(GCC_OPTIONS) $(SRC)/samples/man_client.c -o $(BIN)/man_client -lpthread

clean: rm -rf *.o
