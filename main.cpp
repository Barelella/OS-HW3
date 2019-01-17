#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>

#include <netinet/in.h>

// Constants
const int WAIT_FOR_PACKET_TIMEOUT = 3;
const int NUMBER_OF_FAILURES = 7;
const int MAX_BUFF_SIZE = 516;
const int OPCODE_SIZE = 2;
const char* USAGE = "USAGE: ./ttftps <port>/n";

typedef struct sockaddr_in IPv4;
typedef struct timeval time;

struct ack_t {
	short opcode;
	short blockNumber;
} __attribute__((packed));
typedef struct ack_t Ack;



int main(int argc, char* argv[])
{
	// Declarations
	int sock, port, messageLen, numOfBytesSent;
	unsigned int clientAddrLen;
	short givenOpcode;
	short givenBlocknum;

	char buffer[MAX_BUFF_SIZE];
	char transMode[MAX_BUFF_SIZE];
	char fileName[MAX_BUFF_SIZE];
	char data[MAX_BUFF_SIZE];

	int timeoutExpiredCount = 0;
	int lastWriteSize = 0;
	int select;
	int nfds;

	IPv4 myAddr, clientAddr;
	FILE* fd;
	Ack ack;
	time timeout;
	
	// Declarations - end
	
	//select varables init.
	fd_set set;
	FD_CLR(fd, &set);
	FD_ISSET(fd, &set);
	FD_SET(fd, &set);
	
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

	time.tv_sec = WAIT_FOR_PACKET_TIMEOUT;

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

		do
		{
			do
			{
				do
				{
					// TODO: Wait WAIT_FOR_PACKET_TIMEOUT to see if something appears
					// for us at the socket (we are waiting for DATA)
					select = select(nfds, set, NULL, NULL, timeout, NULL);
					messageLen = recvfrom(sock, buffer, MAX_BUFF_SIZE, 0, (struct sockaddr *)&clientAddr, &clientAddrLen);
					if (messageLen>0) //if there is something at the socket
					{
						// Read the DATA packet from the socket
						memcpy(&givenOpcode, buffer, OPCODE_SIZE);
						givenOpcode = ntohs(givenOpcode);
						memcpy(&givenBlocknum, buffer + OPCODE_SIZE, OPCODE_SIZE);
						givenBlocknum = ntohs(givenBlocknum);
					}
					if (messageLen=0) // TODO: Time out expired while waiting for data
							 // to appear at the socket
					{
						// Send another ACK for the last packet
						numOfBytesSent = sendto(sock, &ack, sizeof(ack), 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
						if (numOfBytesSent != sizeof(ack)) {
							perror("TTFTP_ERROR: An error occurred while sending ACK to client\n");
							exit(1);
						}
						printf("OUT:ACK, %d\n", ack.blockNumber);
						timeoutExpiredCount++;
					}
					if (timeoutExpiredCount >= NUMBER_OF_FAILURES)
					{
						printf("FLOW_ERROR: Number of missing packets exceeded limit\n");
						exit(1);
					}
				} while (messageLen<0); // recvfrom failed to read the data
				if (givenOpcode != 3) // We got something else but DATA
				{
					printf("FLOWERROR: Recieved packet does not contain data\n");
					exit(1);
				}
				if (givenBlocknum != (ack.blockNumber + 1)) // Block number in DATA is wrong
				{
					printf("FLOWERROR: Block number error\n");
					exit(1);
				}
				else
				{
					strcpy(data, buffer + 2 * OPCODE_SIZE);
					ack.blockNumber++;
					printf("OUT:ACK, %d\n", ack.blockNumber);
					break;
				}
			} while (false);
			timeoutExpiredCount = 0;
			lastWriteSize = fwrite(buffer, sizeof(char), sizeof(buffer), fd); // write next bulk of data
			//send ACK packet to the client
			numOfBytesSent = sendto(sock, &ack, sizeof(ack), 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
			if (numOfBytesSent != sizeof(ack)) {
				perror("TTFTP_ERROR: An error occurred while sending ACK to client\n");
				exit(1);
			}
			printf("OUT:ACK, %d\n", ack.blockNumber);
		} while (false); // TODO: Have blocks left to be read from client (not end of transmission)

	} while (true);

	close(sock);
	return 0;
}
