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
	printf("-port The port number to bind the server listener to.  Default is %d\n\n",  DEFAULT_SERVER_PORT);

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
long int total_messages = 0;


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

static inline void increment_total() {
	lock();
	total_messages++;
	unlock();
}

bool validate_xml () {

	return true;
}

void print_command_xml_field_and_date(client_message message) {
	// From requirements:'Parse the command field out of the XML and display it to the console along with the receive date
	printf("processing message,  client_socket=%d, timestamp=%ld, total_messages=%ld\n message=%s\n\n",
	message.client_socket, message.receive_timestamp, total_messages, message.message);
	//Is it ok to not lock when reading total_messages?  I'm not too worried :)

	//Sucessfully processed message, so update total message count
	increment_total();


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
	//TODO add getopt and parse cmd line args
	//print_usage();
	
	struct sockaddr_in address;
	const int opt = 1;
	int addrlen = sizeof(address);
	int serversocket_fd, new_socket;

	if ((serversocket_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		perror("Create socket failed");
		exit(EXIT_FAILURE);
	}

	if (setsockopt(serversocket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
		perror("setsockopt failed");
		exit(EXIT_FAILURE);
	    }
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(DEFAULT_SERVER_PORT);

	if (bind(serversocket_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

	if (listen(serversocket_fd, WORK_QUEUE_DEPTH) < 0) {
		perror("listen failed");
		exit(EXIT_FAILURE);
	}

	// Create threads before calling accept, there might be messages to process right away
	for (int i = 0; i < WORKER_THREAD_COUNT; i++) {
		if (pthread_create(&worker_pool[i], NULL, thread_main, NULL) != 0) {
			perror("Failed to create worker threads");
			return 1;
		}
	}

	printf("Server and threads started.  Listening on port %d\n",DEFAULT_SERVER_PORT);
	while (1) {
		if ((new_socket = accept(serversocket_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
			perror("accept failed");
			exit(EXIT_FAILURE);
		}

		printf("Loop: new accept, socket fd is %d, IP is: %s, port: %d\n", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

		// Get message payload
		client_message new_message;
		new_message.client_socket = new_socket;
		new_message.receive_timestamp = time(NULL);
		int valread = recv(new_socket, new_message.message, BUFFER_SIZE, 0);
		if (valread < 0) {
			perror("recv failed, packet dropped");
			close(new_socket);
			continue;
		}
		new_message.message[valread] = '\0'; // Make SURE it's null terminated

		// Add the client data to the queue for processing on a worker thread
		if (!enqueue(new_message)) {
			perror("Work Queue is full, dropping a valid packet because out of queue room\n");
			perror("Either increase queue depth or add more/faster worker threads because they are behind\n");
			close(new_socket);
			continue;
		}
	}
	
	//Cleanup, will we ever get here?   Does it even matter?
	for (int i = 0; i < WORKER_THREAD_COUNT; i++) {
		pthread_join(worker_pool[i], NULL);
	}
	    
	close(serversocket_fd);

	return 0;
}
