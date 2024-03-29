#ifndef SETTINGS_GUARD
#define SETTINGS_GUARD

#define FAILURE 0
#define SUCCESS 1
#define DEFAULT_CONFIG_FILENAME "routing_big.cfg"
#define MIN_ID 1
#define MAX_ID 255
#define MAX_NODES (MAX_ID - MIN_ID)
#define MAX_LINKS 1000
#define MAX_CONNECTIONS 8

#define HELLO_TIMER 1
#define DEATH_TIMER 3
#define DDR_TIMER 30

#define BUF_SIZE 200
#define MAX_MSG_LEN 150
#define RESEND_BUFFER_SIZE 5

//#define DEBUG 1

#define IP_ADDRESS_MAX_LENGTH 16
#endif
