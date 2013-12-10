#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include "buffer.h"

void showUndeliveredMessage(UndeliveredMessage *ptr)
{
	if(ptr==NULL) return;
	fprintf(stderr, "dest_id: %d\tmsg (%d bytes): %s\n", ptr->dest_id, ptr->len, ptr->message+3);
}

void showUndeliveredMessages(struct shared_mem *mem)
{
	pthread_mutex_lock(&(mem->mutexes->buffer_mutex));
	fprintf(stderr, "-----------\n");
	fprintf(stderr,"Undelivered messages buffer:\n");
	fprintf(stderr,"-----------\n");
	UndeliveredMessage *ptr = mem->buffer;

	while(ptr!=NULL){
		showUndeliveredMessage(ptr);
		ptr = ptr->next;
	}
	fprintf(stderr,"-----------\n");
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
				//leave in buffer if send wasn't successful.
				continue;
			}
			//delete from buffer
#ifdef DEBUG
			fprintf(stderr,"will delete this msg(sent): ");
			showUndeliveredMessage(*ptr);
#endif

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
	fprintf(stderr,"will delete (buffer full): ");
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
	// prepare the message
	new->dest_id = dest_id;
	new->len = len;
	new->message = (char *)calloc(len, sizeof(char));
	memcpy(new->message, msg, len*sizeof(char));
	new->next = NULL;

#ifdef DEBUG
	fprintf(stderr,"adding into buffer: ");
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
	} 
	pthread_mutex_unlock(&(mem->mutexes->buffer_mutex));
}
