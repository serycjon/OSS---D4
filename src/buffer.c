#include<stdlib.h>
#include <string.h>
#include<stdio.h>

#define RESEND_BUFFER_SIZE 4

typedef struct undelivered_message {
	int dest_id;
	int len;
	char* message;
	struct undelivered_message *next;
} UndeliveredMessage;

struct shared_mem{
	int buf_size;
	UndeliveredMessage *buffer;
};

void showUndeliveredMessage(UndeliveredMessage *ptr){
	if(ptr==NULL) return;
	printf("dest_id: %d\tmsg: %s\n", ptr->dest_id, ptr->message);
}
void showUndeliveredMessages(struct shared_mem *mem){
	printf("-----------\n");
	printf("Undelivered messages buffer:\n");
	printf("-----------\n");
	UndeliveredMessage *ptr = mem->buffer;

	while(ptr!=NULL){
		showUndeliveredMessage(ptr);
		ptr = ptr->next;
	}
	printf("-----------\n");
}

void resendUndeliveredMessages(int to_id, struct shared_mem *mem){
	UndeliveredMessage *ptr = mem->buffer;
	UndeliveredMessage *prev = mem->buffer;


	while(ptr != NULL){
		if(ptr->dest_id == to_id){
			//send
			UndeliveredMessage *next = ptr->next;
			free(ptr->message);
			free(ptr);
			*ptr = next;
			mem->buf_size--;
		}
		ptr = &(*ptr)->next;
	}
}

void deleteOldMessage(struct shared_mem *mem){
	UndeliveredMessage **ptr = &(mem->buffer->next);
	while((*ptr)->next != NULL) ptr = &(*ptr)->next; // move to the end
	printf("will delete: ");
	showUndeliveredMessage(*ptr);
	free((*ptr)->message);
	free((*ptr));
	(*ptr) = NULL;
	mem->buf_size--;
}
void addWaitingMessage(int dest_id, int len, char* msg, struct shared_mem *mem){
	UndeliveredMessage *new = (UndeliveredMessage *)malloc(sizeof(UndeliveredMessage));
	new->dest_id = dest_id;
	new->len = len;
	new->message = (char *)calloc((len+1), sizeof(char));
	memcpy(new->message, msg, len*sizeof(char));
	new->next = NULL;

	printf("adding: ");
	showUndeliveredMessage(new);

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
}



int main(){
	struct shared_mem *mem = (struct shared_mem *) malloc(sizeof(struct shared_mem));
	mem->buffer = NULL;
	mem->buf_size = 0;

	char *msg = "Ahoj!";
	char *msg2 = "nazdar!";
	char *msg3 = "cauky!";
	char *msg4 = "jupii!";
	//UndeliveredMessage *head = (UndeliveredMessage *)malloc(sizeof(UndeliveredMessage));
	addWaitingMessage(1, strlen(msg), msg, mem);
	addWaitingMessage(2, strlen(msg2), msg2, mem);
	addWaitingMessage(2, strlen(msg3), msg3, mem);
	addWaitingMessage(4, strlen(msg4), msg4, mem);
	
	//addWaitingMessage(1, strlen(msg), msg, mem);
	showUndeliveredMessages(mem);
	resendUndeliveredMessages(2, mem);
	showUndeliveredMessages(mem);
}
