#include "disconnect.h"

extern int fd;
extern int n_seq;

extern volatile int STOP;
extern int alarm_enabled;
extern int timeout_count;

State state;

void stateMachineDisc(State * state, unsigned char byte, LinkLayer connectionParameters)
{
    unsigned char A;
    unsigned char C = CONTROL_DISC;

    if (connectionParameters.role == LlTx) {
        A = RECEIVER_REPLY;
    }
    else if (connectionParameters.role == LlRx) {
        A = TRANSMITTER_COMMAND;
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
    }
    default:
        break;
}

int transmitterDisc(unsigned char packet[], LinkLayer connectionParameters) 
{
    (void)signal(SIGALRM, alarmHandler);
    (void)siginterrupt(SIGALRM,TRUE); //system call interrupted by alarm isn't restarted

    unsigned char in_char;

    timeout_count = 0;
    alarm_enabled = FALSE;
    state = StateSTART;

    while (state != StateSTOP && timeout_count < connectionParameters.nRetransmissions)
    {

        if (alarm_enabled == FALSE)
        {
            write(fd, packet, SU_BUF_SIZE);
            sleep(1);
            printf("Sent disc frame\n");
            alarm(connectionParameters.timeout); // Set alarm
            state = StateSTART;
            alarm_enabled = TRUE;
        }

        read(fd, &in_char, 1);
        stateMachineDisc(&state, in_char, connectionParameters);
    }

    alarm(0);

    if(timeout_count == connectionParameters.nRetransmissions){
        printf("Max timeouts exceeded\n");
        exit(-1);
    }
    else{
        printf("Received Disc frame\n");
        printf("Disconnecting\n");
    }
    return 0;  
}

int receiverDisc(unsigned char packet[], LinkLayer connectionParameters) {

    unsigned char in_char;

    state = StateSTART;

    while (state != StateSTOP)
    {
        read(fd, &in_char, 1); 
        stateMachineDisc(&state, in_char, connectionParameters);
    }
    printf("Received Disc frame\n");

    write(fd, packet, SU_BUF_SIZE);
    sleep(1);
    printf("Sent Disc frame\n");

    printf("Disconnecting\n");

    return 0;
}
