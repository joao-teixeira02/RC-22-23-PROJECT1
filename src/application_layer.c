// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include <signal.h>

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

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{

    LinkLayer connectionparameters;
    connectionparameters.role = role;
    connectionparameters.baudRate = baudRate;
    *connectionparameters.serialPort = serialPort; 
    connectionparameters.nRetransmissions = nTries;
    connectionparameters.timeout = timeout;

    int fd = llopen(connectionparameters);

    unsigned char in_char[] = {0};
    unsigned char out_char[] = {0};

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
}

// Alarm function handler
void alarmHandler(int signal)
{
    alarm_enabled = FALSE;
    state = StateSTART;
    timeout_count++;
    printf("Timeout #%d\n", timeout_count);
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