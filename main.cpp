#include "ttftp.h"

const char* USAGE = "USAGE: ./ttftps <port>\n";

int main(int argc, char* argv[])
{
	// Declarations
	int sock, port;

	IPv4 myAddr, clientAddr;
	
	// Declarations - end
	
	if (2 != argc){
		printf(USAGE);
		return 1;
	}

	// Open Socket
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		perror("TTFTP_ERROR: Could not open new socket\n");
		return 1;
	}

	port = atoi(argv[1]);
	memset(&myAddr, 0x0, sizeof(myAddr));
	myAddr.sin_family = AF_INET;
	myAddr.sin_addr.s_addr = INADDR_ANY;
	myAddr.sin_port = htons(port);

	// Bind socket to port
	if (bind(sock, (struct sockaddr *)&myAddr, sizeof(myAddr)) < 0) {
		perror("TTFTP_ERROR: Could not bind socket\n");
		close(sock);
		return 1;
	}

	do
	{
        portListen(sock, &clientAddr);//added function so when a system error occures we can go back to
                           // listen to the port;
	} while (true);

	close(sock);
	return 0;
}
