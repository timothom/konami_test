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
	char *payload_file;

	if (argc == 2) {
		//Use default IP and port
		server_address = DEFAULT_SERVER_IP;
		port = DEFAULT_SERVER_PORT;
		payload_file =  argv[1];		
	} else if (argc == 4) {
		server_address = argv[1];
		port = atoi(argv[2]);
		payload_file = argv[3];	
	} else {
		printf("Usage: %s <server_address> <port> <file>\n", argv[0]);
		printf("Server address is an IPv4 address, default address is %s\n", DEFAULT_SERVER_IP);
		printf("Port is an integer, default is %d\n\n", DEFAULT_SERVER_PORT);
		printf("File to send as the message payload.  Should be XML data\n");
		return 1;
	}

	// Read the file in
	FILE *xml_file;
	char buffer[BUFFER_SIZE] = {0};
	long file_size;

	xml_file = fopen(payload_file, "r");
	if (!xml_file) {
		fprintf(stderr, "Error opening file, file was %s\n", payload_file);
		exit(EXIT_FAILURE);
	}

	if (fseek(xml_file, 0, SEEK_END) != 0) {
		fprintf(stderr, "Error reading to end of file\n");
		fclose(xml_file);
		exit(EXIT_FAILURE);
	}

	file_size = ftell(xml_file);
	if (file_size == -1) {
		fprintf(stderr, "Error getting file size\n");
		fclose(xml_file);
		exit(EXIT_FAILURE);
	}
	
	if (file_size > BUFFER_SIZE) {
		fprintf(stderr, "File size was %lu bytes, this is too large for buffer size of %d \n", file_size, BUFFER_SIZE);
		//If you see this error, increase BUFFER_SIZE and rebuild
		fclose(xml_file);
		exit(EXIT_FAILURE);
	}

	if (fseek(xml_file, 0, SEEK_SET) != 0) {
		fprintf(stderr, "Error seeking back to start of the  file....?\n");
		fclose(xml_file);
		exit(EXIT_FAILURE);
	}

	size_t bytes_read = fread(buffer, 1, file_size, xml_file);
	if (bytes_read != file_size) {
		fprintf(stderr, "Error, bytes read %ld was not the same as file size %ld ?\n", bytes_read, file_size);
		fclose(xml_file);
		exit(EXIT_FAILURE);
	}
	
	buffer[bytes_read] = '\0'; //Make sure it's null terminated
	fclose(xml_file);
	
	// Socket setup and send the file payload
	int socket_fd = 0;
	struct sockaddr_in serv_addr;

	if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "Create socket failed\n");
		exit(EXIT_FAILURE);
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);

	if (inet_pton(AF_INET, server_address, &serv_addr.sin_addr) <= 0) {
		fprintf (stderr, "Server address is invalid\n");
		exit(EXIT_FAILURE);
	}

	if (connect(socket_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		fprintf(stderr, "Connection failed\n");
		exit(EXIT_FAILURE);
	}

	//printf("Connected to server at %s:%d\n", server_address, port);

	send(socket_fd, buffer, bytes_read, 0);
	//printf("Sent: %s\n", payload);

	//Re-use the buffer to hold the response
	memset(buffer, 0, sizeof(buffer));

	#pragma GCC diagnostic ignored "-Wunused-variable"  //Let me comment out my handy debug printf in peace
	int read_rc = read(socket_fd, buffer, BUFFER_SIZE);
	//printf("Response: %s  rc=%d\n", buffer, read_rc);

	if (strcmp(buffer, MESSAGE_ACK_CODE)) {
		printf("Sever error: Message was not accepted by server\n");
	}
	close(socket_fd);
	return 0;
}


