//
// Created ELLA on 1/18/19.
//

#ifndef OS_HW3_TTFTP_H
#define OS_HW3_TTFTP_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>


#include <netinet/in.h>

// Constants
const int WAIT_FOR_PACKET_TIMEOUT = 3;
const int NUMBER_OF_FAILURES = 7;
const int MAX_BUFF_SIZE = 516;
const int OPCODE_SIZE = 2;
const char* USAGE = "USAGE: ./ttftps <port>/n";

typedef struct sockaddr_in IPv4;
typedef struct timeval time;
typedef struct ack_t Ack;

void portListen(int sock, IPv4* clientAddr);

#endif //OS_HW3_TTFTP_H
