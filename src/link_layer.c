// Link layer protocol implementation

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


// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1

#define BUF_SIZE 256
#define FLAG 0x7E
#define SET 0x03
#define UA 0x07
#define TRANSMITTER_COMMAND 0x03
#define RECEIVER_REPLY 0x03
#define RECEIVER_COMMAND 0x01
#define TRANSMITTER_REPLY 0x01
#define SU_BUF_SIZE 5

#define TIMEOUT_SECS 3
#define MAX_TIMEOUTS 3

typedef enum{
    StateSTART,
    StateFLAG,
    StateA,
    StateC,
    StateBCC,
    StateSTOP
} State;

State state;

volatile int STOP = FALSE;
int alarm_enabled = FALSE;
int timeout_count = 0;

struct termios oldtio;
struct termios newtio;

// Alarm function handler
void alarmHandler(int signal)
{
    alarm_enabled = FALSE;
    state = StateSTART;
    timeout_count++;
    printf("Timeout #%d\n", timeout_count);
}

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{

    int fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        perror(connectionParameters.serialPort);
        exit(-1);
    }


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

    if(role == "tx"){
        (void)signal(SIGALRM, alarmHandler);
        (void)siginterrupt(SIGALRM,TRUE); //system call interrupted by alarm isn't restarted
        
        while (state != StateSTOP && timeout_count < MAX_TIMEOUTS)
        {

            if (alarm_enabled == FALSE)
            {
                llwrite(fd, out_char, 1);
                alarm(TIMEOUT_SECS); // Set alarm
                alarm_enabled = TRUE;
            }

            // Returns after 1 chars has been input
            llread(fd, &in_char);        
            stateMachine(&state, in_char);

        }

        alarm(0);

        if(timeout_count == MAX_TIMEOUTS){
            printf("Max timeouts exceded\n");
        }
        else{
            printf("Received unnumbered acknowledgment frame\n");
            printf("Connection established\n");
        }

    }else if (role == "rx"){

        while (state != StateSTOP)
        {
            // Returns after 1 chars has been input
            llread(fd, &out_char);        
            stateMachine(&state, out_char);
        }
        
        printf("Received set up frame\n");
        
        unsigned char bcc[5] = {1,2,3,4,5};
        
        llwrite(fd, bcc, 5);
        printf("Sent unnumbered acknowledgment frame\n");

        // Wait until all bytes have been written to the serial port
        sleep(1);
        printf("Connection established\n");
    }

    return fd;
}

void stateMachine(State * state, unsigned char byte[])
{

    unsigned char A = RECEIVER_REPLY;
    unsigned char C = UA;
    
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

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(int fd, const unsigned char *buf, int bufSize)
{
    int ret = write(fd, buf, bufSize);
    sleep(1);
    return ret;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(int fd, unsigned char *packet)
{
    int ret = read(fd, packet, sizeof(&packet));
    sleep(1);
    return ret;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int fd, int showStatistics)
{
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    return close(fd); 
}
