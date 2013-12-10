#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include "buffer.h"



// struct shared_mem{
// 	int buf_size;
// 	UndeliveredMessage *buffer;
// };

void showUndeliveredMessage(UndeliveredMessage *ptr)
{
	if(ptr==NULL) return;
	printf("dest_id: %d\tmsg (%d bytes): %s\n", ptr->dest_id, ptr->len, ptr->message);
}

void showUndeliveredMessages(struct shared_mem *mem)
{
	pthread_mutex_lock(&(mem->mutexes->buffer_mutex));
	printf("-----------\n");
	printf("Undelivered messages buffer:\n");
	printf("-----------\n");
	UndeliveredMessage *ptr = mem->buffer;

	while(ptr!=NULL){
		showUndeliveredMessage(ptr);
		ptr = ptr->next;
	}
	printf("-----------\n");
	pthread_mutex_unlock(&(mem->mutexes->buffer_mutex));
}

void resendUndeliveredMessages(int to_id, struct shared_mem *mem)
{
	pthread_mutex_lock(&(mem->mutexes->buffer_mutex));
	UndeliveredMessage **ptr = &(mem->buffer);

	while(*ptr != NULL){
		if((*ptr)->dest_id == to_id || to_id==ALL){
			//send
			if(sendToId((*ptr)->dest_id, (*ptr)->message, (*ptr)->len, mem, NORETRY) == -1){
				ptr = &(*ptr)->next;
				continue;
			}
			//sleep(1);

			//delete from buffer
// #ifdef DEBUG
			printf("will delete this msg(sent): ");
			showUndeliveredMessage(*ptr);
// #endif
			UndeliveredMessage *old = ((*ptr));
			*ptr = (*ptr)->next;

			free(old->message);
			free(old);
			mem->buf_size--;
		}else{
			ptr = &(*ptr)->next;
		}
	}
	pthread_mutex_unlock(&(mem->mutexes->buffer_mutex));
}

void deleteOldMessage(struct shared_mem *mem)
{
	UndeliveredMessage **ptr = &(mem->buffer->next);
	while((*ptr)->next != NULL) ptr = &(*ptr)->next; // move to the end
#ifdef DEBUG
	printf("will delete: ");
	showUndeliveredMessage(*ptr);
#endif
	free((*ptr)->message);
	free((*ptr));
	(*ptr) = NULL;
	mem->buf_size--;
}

void addWaitingMessage(int dest_id, int len, char* msg, struct shared_mem *mem)
{
	UndeliveredMessage *new = (UndeliveredMessage *)malloc(sizeof(UndeliveredMessage));
	new->dest_id = dest_id;
	new->len = len;
	new->message = (char *)calloc(len, sizeof(char));
	memcpy(new->message, msg, len*sizeof(char));
	new->next = NULL;

#ifdef DEBUG
	printf("adding: ");
	showUndeliveredMessage(new);
#endif

	pthread_mutex_lock(&(mem->mutexes->buffer_mutex));
	if(mem->buf_size == 0){
		mem->buffer = new;
		mem->buf_size++;
	}else{
		UndeliveredMessage *old = (mem->buffer);
		if (mem->buf_size >= RESEND_BUFFER_SIZE){
			deleteOldMessage(mem);
		}
		new->next = old;
		mem->buffer = new;
		mem->buf_size++;

		//linkListHeadInsert
	} 
	pthread_mutex_unlock(&(mem->mutexes->buffer_mutex));
}



// not needed


int test()
{
	struct shared_mem *mem = (struct shared_mem *) malloc(sizeof(struct shared_mem));
	mem->buffer = NULL;
	mem->buf_size = 0;

	char *msg = "Ahoj!";
	char *msg2 = "nazdar!";
	char *msg3 = "cauky!";
	char *msg4 = "jupii!";
	//UndeliveredMessage *head = (UndeliveredMessage *)malloc(sizeof(UndeliveredMessage));
	
	addWaitingMessage(2, strlen(msg4), msg4, mem);
	addWaitingMessage(1, strlen(msg), msg, mem);
	addWaitingMessage(2, strlen(msg3), msg3, mem);
	addWaitingMessage(2, strlen(msg2), msg2, mem);

	//addWaitingMessage(1, strlen(msg), msg, mem);
	showUndeliveredMessages(mem);
	resendUndeliveredMessages(2, mem);
	showUndeliveredMessages(mem);
	return 0;
}
