#ifndef BUFFER_GUARD
#define BUFFER_GUARD

#include "main.h"

#define ALL -1

void showUndeliveredMessage(UndeliveredMessage *ptr);
void showUndeliveredMessages(struct shared_mem *mem);
void resendUndeliveredMessages(int to_id, struct shared_mem *mem);
void deleteOldMessage(struct shared_mem *mem);
void addWaitingMessage(int dest_id, int len, char* msg, struct shared_mem *mem);
// not needed
int test();

#endif
