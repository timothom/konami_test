/* Some copyright */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <arpa/inet.h>

#include "constants.h"
#include "yxml.h"

//*****Globals*****
pthread_mutex_t main_lock = PTHREAD_MUTEX_INITIALIZER; //A spinlock would be faster, but a mutex prevents head-of-line blocking
pthread_cond_t main_wait = PTHREAD_COND_INITIALIZER;
pthread_t worker_pool[WORKER_THREAD_COUNT];

// Note: these are globals, so hold the lock before read/write
// Server listener will add messages to the tail
// Worker threads will pull messages off the head
// Head chases the tail
volatile client_message queue[WORK_QUEUE_DEPTH];
volatile int queue_head = 0, queue_tail = 0, queue_size = 0;
volatile long int total_messages = 0;
// Volatile is likely overkill for these globals

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
	//lock();
	//TODO this accumulator should have its own lock
	total_messages++;
	//unlock();
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

bool validate_xml (client_message message) {
	char parser_stack[BUFFER_SIZE];
	yxml_ret_t rc;
	yxml_t yxml_parser;
	yxml_init(&yxml_parser, parser_stack, sizeof(parser_stack));

	int i = 0;
	while (message.message[i]) {
		rc = yxml_parse(&yxml_parser, message.message[i]);
		if (rc < 0) {
			// Parser found an error or got confused, so this is not valid XML
			return false;
		}
		i++;
	}
	return true;
}

void print_command_xml_field_and_date(client_message message) {
	// From requirements:'Parse the command field out of the XML and display it to the console along with the receive date

	/* printf("processing message,  client_socket=%d, timestamp=%ld, total_messages=%ld\n message=%s\n\n",
	message.client_socket, message.receive_timestamp, total_messages, message.message); */

	char parser_stack[BUFFER_SIZE];
	char payload_buffer[BUFFER_SIZE] = {0};
	yxml_ret_t rc;
	yxml_t yxml_parser;
	yxml_init(&yxml_parser, parser_stack, sizeof(parser_stack));

	for (int i=0; i < sizeof(payload_buffer); i++) {
		rc = yxml_parse(&yxml_parser, message.message[i]);
		if (!strncmp(yxml_parser.elem, MESSAGE_TARGET_FIELD, sizeof(MESSAGE_TARGET_FIELD))) { //Command field found
			//Drain the command field value and print it out along with the  time of day to fullfill a XML requirement
			int j = 0;
			//Found the first element of the payload
			do {
				payload_buffer[j] = yxml_parser.data[0];
				//printf("%s:%d", yxml_parser.data, rc);
				rc = yxml_parse(&yxml_parser, message.message[++i]);
				j++;
			} while (rc == YXML_CONTENT || rc == YXML_ELEMSTART);
			char time_string[128] = {0};
			strftime(time_string, 80, "%Y-%m-%d", localtime(&message.receive_timestamp));
			if (!BENCHMARK)
				printf("Command Field Value=%s  Rx Time=%s\n", payload_buffer, time_string);			
			return;			
		}	
	}
	//Sucessfully processed message, so update total message count
	increment_total();
}

// The function the worker threads run
void *thread_main(void *arg) {
	while (1) {
		client_message message = dequeue();
		assert(message.client_socket);

		//Process the message
		print_command_xml_field_and_date(message);

		//Simulate a bit more work
		if (!BENCHMARK) {
			sleep(TASK_DELAY_TIME);
		}

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
	char *server_address;
	int port;
	if (argc == 1) {
		//Use default IP and port
		server_address = DEFAULT_SERVER_IP;
		port = DEFAULT_SERVER_PORT;		
	} else if (argc == 3) {
		server_address = argv[1];
		port = atoi(argv[2]);
	} else {
		printf("Usage: %s <server_address> <port>\n", argv[0]);
		printf("Server address is an IPv4 address, default address is %s\n", DEFAULT_SERVER_IP);
		printf("Port is an integer, default is %d\n\n", DEFAULT_SERVER_PORT);
		exit(EXIT_FAILURE);
	}
	
	struct sockaddr_in address;
	const int opt = 1;
	int addrlen = sizeof(address);
	int serversocket_fd, new_socket;

	if (inet_pton(AF_INET, server_address, &address.sin_addr) <= 0) {
		fprintf (stderr, "Server address is invalid\n");
		exit(EXIT_FAILURE);
	}

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
	address.sin_port = htons(port);

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
			exit(EXIT_FAILURE);
		}
	}

	printf("Server and threads started.  Listening on port %d\n\n", port);
	while (1) {
		if ((new_socket = accept(serversocket_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
			perror("accept failed");
			exit(EXIT_FAILURE);
		}

		//printf("Loop: new accept, socket fd is %d, IP is: %s, port: %d\n", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

		// Get message payload
		client_message new_message;
		new_message.client_socket = new_socket;
		new_message.receive_timestamp = time(NULL);
		int valread = recv(new_socket, new_message.message, BUFFER_SIZE, 0);
		if (valread < 0) {
			fprintf(stderr,"recv failed, packet dropped");
			close(new_socket);
			continue;
		}

		new_message.message[valread] = '\0'; // Make SURE it's null terminated

		if  (!validate_xml(new_message)) {
			//Not valid XML, don't enqueue this for processing
			//Per requirements, display 'Unknown Command' in the console if an XML message isn't valid XML
			fprintf(stdout, "\"Unknown Command\"\n");
			//We could count failures
			close(new_socket);
			continue;
		}

		// Add the client data to the queue for processing on a worker thread
		if (!enqueue(new_message)) {
			fprintf(stderr, "Work Queue is full, dropping a valid packet because out of queue room\n");
			// This is bad.  Either increase queue depth or add more/faster worker threads because they are behind
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
