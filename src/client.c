/* Some copyright */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "constants.h"


int main(int argc, char* argv[]) {
	char *server_address;
	int port;
	char *payload;

	if (argc == 2) {
		//Use default IP and port
		server_address = DEFAULT_SERVER_IP;
		port = DEFAULT_SERVER_PORT;
		payload =  argv[1];		
	} else if (argc == 4) {
		server_address = argv[1];
		port = atoi(argv[2]);
		payload = argv[3];	
	} else {
		printf("Usage: %s <server_address> <port> <payload>\n", argv[0]);
		printf("Server address is an IPv4 address, default address is %s\n", DEFAULT_SERVER_IP);
		printf("Port is an integer, default is %d\n\n", DEFAULT_SERVER_PORT);
		return 1;
	}

	int socket_fd = 0;
	struct sockaddr_in serv_addr;
	char buffer[BUFFER_SIZE] = {0};

	if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Create socket failed\n");
		exit(EXIT_FAILURE);
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);

	if (inet_pton(AF_INET, server_address, &serv_addr.sin_addr) <= 0) {
		perror("Server address is invalid\n");
		exit(EXIT_FAILURE);
	}

	if (connect(socket_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		perror("Connection failed\n");
		exit(EXIT_FAILURE);
	}

	//printf("Connected to server at %s:%d\n", server_address, port);

	send(socket_fd, payload, strlen(payload), 0);
	//printf("Sent: %s\n", payload);

	int read_rc = read(socket_fd, buffer, BUFFER_SIZE);
	printf("Response: %s  rc=%d\n", buffer, read_rc);

	close(socket_fd);

	return 0;
}


