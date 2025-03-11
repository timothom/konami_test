/* Some copyright */

#include <stdio.h>

#include "constants.h"


void print_usage() {
	printf("\nUsage: ./server -ip aaa.bbb.ccc.ddd -port N\n");
	printf("-ip The IPv4 address to run the server on.  Default is %s\n", DEFAULT_SERVER_IP);
	printf("-port The port number to bind the server listener to.  Default is %d\n\n\n",  DEFAULT_SERVER_PORT);

}




int main(int argc, char* argv[]) {
	print_usage();
	
	return 0;
}



