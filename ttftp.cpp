//
// Created by ELLA on 1/18/19.
//

#include "ttftp.h"

struct ack_t {
    short opcode;
    short blockNumber;
} __attribute__((packed));

struct data_t {
    short opcode;
    short blockNumber;
    char data[MAX_DATA_SIZE];
} __attribute__((packed));

void portListen(int sock, IPv4* clientAddr){
    // Declarations
    unsigned int clientAddrLen;
    int messageLen, numOfBytesSent;
    short givenOpcode, givenBlocknum;

    char buffer[MAX_BUFF_SIZE];
    char transMode[MAX_BUFF_SIZE];
    char fileName[MAX_BUFF_SIZE];

    int timeoutExpiredCount = 0;
    int lastWriteSize = 0;
    int select_val;

    FILE* fd;
    Ack ack;
    Data data_s;
    time timeout;

    //select varables init.
    fd_set set;
    FD_CLR(sock, &set);
    FD_ISSET(sock, &set);
    FD_SET(sock, &set);

    // UDP handshake
    // Wait for WRQ

    clientAddrLen = sizeof(clientAddr);

    messageLen = (int)recvfrom(sock, buffer, MAX_BUFF_SIZE, 0, (struct sockaddr *)&clientAddr, &clientAddrLen);
    if (messageLen < 0) {
        perror("TTFTP_ERROR: Error reading from socket\n");
        printf("RECVFAIL\n");
        return;
    }

    // Process WRQ
    memcpy(&givenOpcode, buffer, OPCODE_SIZE);
    givenOpcode = (short)ntohs(givenOpcode);
    printf("opcode:%d\n",(int)givenOpcode);//debug print
    int transModeOffsetInWrq = OPCODE_SIZE + strlen(buffer + OPCODE_SIZE) + 1;
    strcpy(transMode, buffer + transModeOffsetInWrq);
    if (givenOpcode != 2 || strcmp(transMode, "octet")) {
        printf("FLOWERROR: Wrong WRQ received\n");
        printf("RECVFAIL\n");
        return; // Back to handshake start, wait for WRQ message all over again
    }

    strcpy(fileName, buffer + OPCODE_SIZE);
    printf("IN:WRQ, %s, octet\n", fileName);
    fd = fopen(fileName, "w");
    if(0 == fd){
        perror("TTFTP_ERROR: Could not open file\n");
        printf("RECVFAIL\n");
        return;
    }

    // send ACK back to client
    ack.opcode = htons(4);
    ack.blockNumber = htons(0);
    numOfBytesSent = (int)sendto(sock, &ack, sizeof(ack), 0, (struct sockaddr *)&clientAddr, sizeof(*clientAddr));
    printf("send result:%d\n", numOfBytesSent);//debug print
    if(numOfBytesSent != sizeof(ack)){
        perror("TTFTP_ERROR: An error occurred while sending WRQ ACK to client\n");
        printf("RECVFAIL\n");
        return;
    }
    printf("OUT:ACK, 0\n");

    do
    {
        do
        {
            // Wait WAIT_FOR_PACKET_TIMEOUT to see if something appears
            // for us at the socket (we are waiting for DATA)
            FD_ZERO(&set);
            FD_SET(sock,&set);
            timeout.tv_sec = WAIT_FOR_PACKET_TIMEOUT;
            timeout.tv_usec = 0;

            printf("starting select\n");//debug print
            select_val = select(sock+1, &set, NULL, NULL, &timeout);
            printf("finished select:%d\n", select_val);//debug print

            if(select_val==-1){
                perror("TTFTP_ERROR: An error occurred while waiting for data\n");
                return;
            }
            else if(select_val==0){// Time out expired while waiting for data
                // to appear at the socket

                // Send another ACK for the last packet
                numOfBytesSent = (int)sendto(sock, &ack, sizeof(ack), 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
                if (numOfBytesSent != sizeof(ack)) {
                    perror("TTFTP_ERROR: An error occurred while sending data ACK to client\n");
                    return;
                }
                printf("OUT:ACK, %d\n", ack.blockNumber);
                timeoutExpiredCount++;

                if (timeoutExpiredCount >= NUMBER_OF_FAILURES)
                {
                    printf("FLOW_ERROR: Number of missing packets exceeded limit\n");
                    printf("RECVFAIL\n");
                    return;
                }
            }
            else{ //there is something at the socket
                printf("socket number before recvfrom:%d\n", sock);//debug print
                messageLen = (int)recvfrom(sock, &data_s, sizeof(data_s), 0, (struct sockaddr *)&clientAddr, &clientAddrLen);//TODO: recvfrom turns sock to 0
                printf("socket number after recvfrom:%d\n", sock);//debug print
                printf("messageLen: %d\n", (int)strlen(buffer));//debug print
                if(messageLen<0){
                    perror("ERROR:");
                    return;
                }
                if(strlen(data_s.data)==0){
                select_val=0;
                }
            }
        } while (select_val<=0);//data hasn't arrived yet
        // Read the DATA packet from the socket
        givenOpcode = (short)ntohs(data_s.opcode);
        printf("opcode:%d\n",givenOpcode);//debug print
        givenBlocknum = (short)ntohs(data_s.blockNumber);
        printf("block number:%d\n",givenBlocknum);//debug print
        if (givenOpcode != 3) // We got something else but DATA
        {
            printf("FLOWERROR: Recieved packet does not contain data\n");
            printf("RECVFAIL\n");
            return;
        }
        if (givenBlocknum != (ack.blockNumber + 1)) // Block number in DATA is wrong
        {
            printf("FLOWERROR: Block number error\n");
            printf("RECVFAIL\n");
            return;
        }
        else
        {
            printf("IN:DATA\n");
            ack.blockNumber++;
            printf("OUT:ACK, %d\n", ack.blockNumber);
        }
        timeoutExpiredCount = 0;
        lastWriteSize = (int)fwrite(data_s.data, sizeof(char), sizeof(data_s), fd); // write next bulk of data
        printf("%s",data_s.data);
        if(lastWriteSize==0){
            perror("TTFTP_ERROR: An error occurred while writing data\n");
            return;
        }
        printf("WRITING: %d\n", (int)strlen(data_s.data));
        //send ACK packet to the client
        numOfBytesSent = sendto(sock, &ack, sizeof(ack), 0, (struct sockaddr *)&clientAddr, sizeof(*clientAddr));
        if (numOfBytesSent != sizeof(ack)) {
            perror("TTFTP_ERROR: An error occurred while sending ACK to client\n");
            return;
        }
        printf("OUT:ACK, %d\n", ack.blockNumber);
    } while (messageLen>0); // TODO: Have blocks left to be read from client (not end of transmission)
    printf("RECVOK");
}
