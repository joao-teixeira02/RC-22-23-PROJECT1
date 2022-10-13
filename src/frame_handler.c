#include "link_layer.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

#define BUF_SIZE 256
#define FLAG 0x7E
#define TRANSMITTER_COMMAND 0x03
#define TRANSMITTER_REPLY 0x01
#define RECEIVER_COMMAND 0x01
#define RECEIVER_REPLY 0x03
#define CONTROL_SET 0x03
#define CONTROL_UA 0x07
#define SU_BUF_SIZE 5

extern fd;

volatile int STOP = FALSE;
int alarm_enabled = FALSE;
int timeout_count = 0;

typedef enum{
    StateSTART,
    StateFLAG,
    StateA,
    StateC,
    StateBCC,
    StateSTOP
} State;

State state;

struct termios oldtio;
struct termios newtio;

void stateMachine(State * state, unsigned char byte[], LinkLayer connectionParameters)
{

    unsigned char A;
    unsigned char C;

    if (connectionParameters.role == 0) {
        unsigned char A = TRANSMITTER_COMMAND;
        unsigned char C = CONTROL_SET;
    }
    else if (connectionParameters.role == 1) {
        unsigned char A = RECEIVER_REPLY;
        unsigned char C = CONTROL_UA;
    }
    else {
        printf("Undefined role in state machine\n");
        exit(-1);
    }

    switch(*state){
    case StateSTART:
        if(byte == FLAG)
            *state = StateFLAG;
        
        break;
    case StateFLAG:
        if(byte == FLAG)
            *state = StateFLAG;
        else if(byte == A)
            *state = StateA;
        else
            *state = StateSTART;
        
        break;
    case StateA:
        if(byte == FLAG)
            *state = StateFLAG;
        else if(byte == C)
            *state = StateC;
        else
            *state = StateSTART;
        
        break;
    case StateC:
        if(byte == FLAG)
            *state = StateFLAG;
        else if(byte == (A^C))
            *state = StateBCC;
        else
            *state = StateSTART;
        
        break;
        
    case StateBCC:
        if(byte == FLAG)
            *state = StateSTOP;
        else
            *state = StateSTART;
        
        break;
    }
}

int setupTermios() {

    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        exit(-1);
    }

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = connectionParameters.baudRate | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0; // Inter-character timer unused
    newtio.c_cc[VMIN] = 1;  // Blocking read until 1 chars received

    // VTIME e VMIN should be changed in order to protect with a
    // timeout the reception of the following character(s)

    // Now clean the line and activate the settings for the port
    // tcflush() discards data written to the object referred to
    // by fd but not transmitted, or data received but not read,
    // depending on the value of queue_selector:
    //   TCIFLUSH - flushes data received but not read.
    tcflush(fd, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");

}

// Alarm function handler
void alarmHandler(int signal)
{
    alarm_enabled = FALSE;
    state = StateSTART;
    timeout_count++;
    printf("Timeout #%d\n", timeout_count);
}


int sendFrame(unsigned char packet[], LinkLayer connectionParameters) 
{

    if(connectionParameters.role == 0){
        (void)signal(SIGALRM, alarmHandler);
        (void)siginterrupt(SIGALRM,TRUE); //system call interrupted by alarm isn't restarted

        while (state != StateSTOP && timeout_count < connectionParameters.nRetransmissions)
        {

            if (alarm_enabled == FALSE)
            {
                llwrite(fd, &packet, 1);
                alarm(connectionParameters.timeout); // Set alarm
                alarm_enabled = TRUE;
            }

            // Returns after 1 chars has been input
            llread(fd, &buf);        
            stateMachine(&state, &buf, connectionParameters);
        }

        alarm(0);

        if(timeout_count == connectionParameters.nRetransmissions{
            printf("Max timeouts exceded\n");
        }
        else{
            printf("Received unnumbered acknowledgment frame\n");
            printf("Connection established\n");
        }
    }
    else if (connectionParameters.role == 1){

        while (state != StateSTOP)
        {
            // Returns after 1 chars has been input
            llread(fd, &buf);        
            stateMachine(&state, &buf, connectionParameters);
        }
        
        printf("Received set up frame\n");
        
        unsigned char UA_packet[SU_BUF_SIZE] = {0};
        
        UA_packet[0] = FLAG;
        UA_packet[1] = RECEIVER_REPLY;
        UA_packet[2] = CONTROL_UA;
        UA_packet[3] = (RECEIVER_REPLY^CONTROL_UA);
        UA_packet[4] = FLAG;
        
        llwrite(fd, UA_packet, SU_BUF_SIZE);
        printf("Sent unnumbered acknowledgment frame\n");

        // Wait until all bytes have been written to the serial port
        sleep(1);
        printf("Connection established\n");
    } 
    else {
        printf("Undefined role passed as argument to llopen()\n");
        exit(-1);
    }
}

int receiveFrame() {

}