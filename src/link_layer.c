// Link layer protocol implementation

#include "link_layer.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>


// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1

#define BUF_SIZE 256
#define FLAG 0x7E
#define TRANSMITTER_COMMAND 0x03
#define TRANSMITTER_REPLY 0x01
#define RECEIVER_COMMAND 0x01
#define RECEIVER_REPLY 0x03
#define CONTROL_SET 0x03
#define CONTROL_UA 0x07
#define SU_BUF_SIZE 5

#define TIMEOUT_SECS 3
#define MAX_TIMEOUTS 3

int fd;

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{

    fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        perror(connectionParameters.serialPort);
        exit(-1);
    }
    
    if (connectionParameters.role == 0) {

        unsigned char SET_packet[SU_BUF_SIZE] = {0}; 

        SET_packet[0] = FLAG;
        SET_packet[1] = TRANSMITTER_COMMAND;
        SET_packet[2] = CONTROL_SET;
        SET_packet[3] = (TRANSMITTER_COMMAND ^ CONTROL_SET);
        SET_packet[4] = FLAG;

        sendFrame(SET_packet, connectionParameters);
        receiveFrame(UA);
    }
    else if (connectionParameters.role == 1) {

        unsigned char UA_packet[SU_BUF_SIZE] = {0};
        
        UA_packet[0] = FLAG;
        UA_packet[1] = RECEIVER_REPLY;
        UA_packet[2] = CONTROL_UA;
        UA_packet[3] = (RECEIVER_REPLY^CONTROL_UA);
        UA_packet[4] = FLAG;

        receiveFrame(SET);
        sendFrame(UA_packet, connectionParameters);
    }
    else {
        printf("Error in llopen: %d is an invalid value for role", connectionParameters.role);
        exit(-1);
    }

    return fd;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    int ret = write(fd, buf, bufSize);
    sleep(1);
    return ret;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    int ret = read(fd, packet, sizeof(&packet));
    sleep(1);
    return ret;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    return close(fd); 
}
