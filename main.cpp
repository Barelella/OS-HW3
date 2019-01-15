#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>

typedef struct sockaddr_in IPv4;

struct ack_t {
	short opcode;
	short blockNumber;
} __attribute__((packed));
typedef struct ack_t Ack;

// Constants
const int WAIT_FOR_PACKET_TIMEOUT = 3;
const int NUMBER_OF_FAILURES = 7;

const int MAX_BUFF_SIZE = 516;
const int OPCODE_SIZE = 2;
const char* USAGE = "USAGE: ./ttftps <port>/n";

int main(int argc, char* argv[])
{
	// Declarations
	int sock, port, messageLen, numOfBytesSent;
	unsigned int clientAddrLen;
	short givenOpcode;

	char buffer[MAX_BUFF_SIZE];
	char transMode[MAX_BUFF_SIZE];
	char fileName[MAX_BUFF_SIZE];

	IPv4 myAddr, clientAddr;
	FILE* fd;
	Ack ack;
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
		return 1;
	}

	do
	{
		// UDP handshake
		// Wait for WRQ
		clientAddrLen = sizeof(clientAddr);
		messageLen = recvfrom(sock, buffer, MAX_BUFF_SIZE, 0, (struct sockaddr *)&clientAddr, &clientAddrLen);
		if (messageLen < 0) {
			perror("TTFTP_ERROR: Error reading from socket\n");
			exit(1);
		}

		// Process WRQ
		memcpy(&givenOpcode, buffer, OPCODE_SIZE);
		givenOpcode = ntohs(givenOpcode);
		int transModeOffsetInWrq = OPCODE_SIZE + strlen(buffer + OPCODE_SIZE) + 1;
		strcpy(transMode, buffer + transModeOffsetInWrq);
		if (givenOpcode != 2 || strcmp(transMode, "octet")) {
			printf("FLOWERROR: Wrong WRQ received\n");
			continue; // Back to handshake start, wait for WRQ message all over again
		}

		strcpy(fileName, buffer + OPCODE_SIZE);
		printf("IN:WRQ, %s, octet\n", fileName);
		fd = fopen(fileName, "w");
		if(0 == fd){
			perror("TTFTP_ERROR: Could not open file\n");
			exit(1);
		}

		// send ACK back to client
		ack.opcode = htons(4);
		ack.blockNumber = 0;
		numOfBytesSent = sendto(sock, &ack, sizeof(ack), 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
		if(numOfBytesSent != sizeof(ack)){
			perror("TTFTP_ERROR: An error occurred while sending ACK to client\n");
			exit(1);
		}
		printf("OUT:ACK, 0\n");

		// From here on everything should be configured OK
		// you should be able to read using recvfrom(sock, ...), see line 72
		// and write using sendto(sock, ...), see line 99
		// note that from here on I didn't touch the code, except that
		// I added default conditions to all IFs and WHILEs, so that the code will compile
		// you obviously may change them as needed

	close(sock);
	return 0;
}
