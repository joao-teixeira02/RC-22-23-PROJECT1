#include "frame_handler.h"

#define BUF_SIZE 256
#define FLAG 0x7E
#define TRANSMITTER_COMMAND 0x03
#define TRANSMITTER_REPLY 0x01
#define RECEIVER_COMMAND 0x01
#define RECEIVER_REPLY 0x03
#define CONTROL_SET 0x03
#define CONTROL_UA 0x07
#define SU_BUF_SIZE 5
#define RECEIVER_READY 0x05

extern int fd;
extern int n_seq;

volatile int STOP = FALSE;
int alarm_enabled = FALSE;
int timeout_count = 0;

State state;

extern struct termios oldtio;
extern struct termios newtio;

void stateMachine(State * state, unsigned char byte, LinkLayer connectionParameters)
{
    unsigned char A;
    unsigned char C;

    if (connectionParameters.role == LlTx) {
        A = RECEIVER_REPLY;
        C = CONTROL_UA;
    }
    else if (connectionParameters.role == LlRx) {
        A = TRANSMITTER_COMMAND;
        C = CONTROL_SET;
    }
    else {
        printf("Undefined role in state machine\n");
        exit(-1);
    }

    switch(*state){
    case StateSTART:
        if(byte == FLAG) {
            *state = StateFLAG;
        }
        break;
    case StateFLAG:
        if(byte == FLAG) {
            *state = StateFLAG;
        }
        else if(byte == A) {
            *state = StateA;
        }
        else {
            *state = StateSTART;
        }
        
        break;
    case StateA:
        if(byte == FLAG) {
            *state = StateFLAG;
        }
        else if(byte == C){
            *state = StateC;
        }
        else {
            *state = StateSTART;
        }
        
        break;
    case StateC:
        if(byte == FLAG){
            *state = StateFLAG;
        }
        else if(byte == (A^C)) {
            *state = StateBCC;
        }
        else {
            *state = StateSTART;
        }
        break;
        
    case StateBCC:
        if(byte == FLAG){
            *state = StateSTOP;
        }
        else {
            *state = StateSTART;
        }
        
        break;

    default:
        break;
    }

}

int setupTermios(LinkLayer connectionParameters) 
{

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
        //exit(-1);
    }

    printf("New termios structure set\n");
    return 0;
}

// Alarm function handler
void alarmHandler(int signal)
{
    alarm_enabled = FALSE;
    timeout_count++;
    printf("Timeout #%d\n", timeout_count);
}

int transmitter(unsigned char packet[], LinkLayer connectionParameters) 
{
    (void)signal(SIGALRM, alarmHandler);
    (void)siginterrupt(SIGALRM,TRUE); //system call interrupted by alarm isn't restarted

    unsigned char in_char;

    while (state != StateSTOP && timeout_count < connectionParameters.nRetransmissions)
    {

        if (alarm_enabled == FALSE)
        {
            write(fd, packet, SU_BUF_SIZE);
            //sleep(1);
            printf("Sent set frame\n");
            alarm(connectionParameters.timeout); // Set alarm
            state = StateSTART;
            alarm_enabled = TRUE;
        }

        read(fd, &in_char, 1);
        stateMachine(&state, in_char, connectionParameters);
    }

    alarm(0);

    if(timeout_count == connectionParameters.nRetransmissions){
        printf("Max timeouts exceeded\n");
        exit(-1);
    }
    else{
        printf("Received unnumbered acknowledgement frame\n");
        printf("Connection established\n");
    }
    return 0;  
}

int receiver(unsigned char packet[], LinkLayer connectionParameters) {

    unsigned char in_char;

    while (state != StateSTOP)
    {
        read(fd, &in_char, 1); 
        stateMachine(&state, in_char, connectionParameters);
    }
    printf("Received set frame\n");

    write(fd, packet, SU_BUF_SIZE);
    //sleep(1);
    printf("Sent unnumbered acknowledgement frame\n");

    printf("Connection established\n");

    return 0;
}

