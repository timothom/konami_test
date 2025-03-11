/* Some copyright */

#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <arpa/inet.h>

#include "constants.h"

void print_usage() {
	printf("\nUsage: ./server -ip aaa.bbb.ccc.ddd -port N\n");
	printf("-ip The IPv4 address to run the server on.  Default is %s\n", DEFAULT_SERVER_IP);
	printf("-port The port number to bind the server listener to.  Default is %d\n\n\n",  DEFAULT_SERVER_PORT);

}

//*****Globals*****
pthread_mutex_t main_lock = PTHREAD_MUTEX_INITIALIZER; //A spinlock would be faster, but a mutex prevents head-of-line blocking
pthread_cond_t main_wait = PTHREAD_COND_INITIALIZER;
pthread_t worker_pool[WORKER_THREAD_COUNT];

// Note: these are globals, so you should hold the lock before you read/write them
// Server listener will add messages to the tail
// Worker threads will pull messages off the head
// Head chases tail
client_message queue[WORK_QUEUE_DEPTH];
int queue_head = 0, queue_tail = 0, queue_size = 0;


//*****Functions******
static inline void lock() {
	if(USE_LOCKING) {
		pthread_mutex_lock(&main_lock);
	}
}

static inline void unlock() {
	if(USE_LOCKING) {
		pthread_mutex_unlock(&main_lock);
	}
}

bool validate_xml () {

	return true;
}

void print_command_xml_field_and_date(client_message message) {
	// From requirements,'Parse the command field out of the XML and display it to the console along with the receive date


}

bool enqueue(client_message message) {
	lock();
	if (queue_size  >= WORK_QUEUE_DEPTH)  {
		//Ohoh, message queue is full, we can't add this message, so drop it
		unlock();
		return false;
	}
	queue[queue_tail] = message;
	queue_tail = (queue_tail + 1) % WORK_QUEUE_DEPTH;
	queue_size++;
	pthread_cond_signal(&main_wait);
	unlock();
	return true;
    }

client_message dequeue() {
	lock();
	while (queue_size == 0) {
		pthread_cond_wait(&main_wait, &main_lock);
	}
	client_message message = queue[queue_head];   //Make a local copy of the message and copy it out, don't return a pointer to the queue message
	queue_head = (queue_head + 1) % WORK_QUEUE_DEPTH;
	queue_size--;
	pthread_cond_signal(&main_wait);
	unlock();
	return message;
}
    
// The function the worker threads run
void *thread_main(void *arg) {
	while (1) {
		client_message message = dequeue();
		assert(message.client_socket);
		//int client_socket = message.client_socket;
		//char *message = data.message;
    
		//printf("Received message: %s from client %d\n", message, client_socket);
		//Process the message
		print_command_xml_field_and_date(message);

		//Simulate a bit more work
		sleep(TASK_DELAY_TIME);

		// Send a ACK back to the client
		send(message.client_socket, MESSAGE_ACK_CODE, strlen(MESSAGE_ACK_CODE), 0);
		close(message.client_socket);
	}
	//Will never get here
	assert(0);
	return NULL;
    }



//***Main***
int main(int argc, char* argv[]) {
	print_usage();
	
	return 0;
}



