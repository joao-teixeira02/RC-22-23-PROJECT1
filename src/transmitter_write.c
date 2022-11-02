#include <transmitter_write.h>

extern int fd;
extern int n_seq;
extern int timeout_count;
extern int alarm_enabled;

State state;

void stateMachineTransmitter(State * state, unsigned char byte)
{

    unsigned char receiver_ready;
    unsigned char receiver_reject;

    if (n_seq == 1) {
        receiver_ready = 0x05;
        receiver_reject = 0x01;
    } else if (n_seq == 0) {
        receiver_ready = 0x85;
        receiver_reject = 0x81;
    } else {
        printf("Error getting value of n_seq in statemachineTransmitter\n");
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
        else if(byte == RECEIVER_REPLY) {
            *state = StateA;
        }
        else {
            *state = StateSTART;
        }
        
        break;
    case StateA:
        if(byte == FLAG) {
            *state = StateFLAG;
        } else if(byte == receiver_ready){
            *state = StateC_RR;
        } else if (byte == receiver_reject){
            *state = StateC_REJ;
        }
        else {
            *state = StateSTART;
        }
        
        break;
    case StateC_RR:
        if(byte == FLAG){
            *state = StateFLAG;
        }
        else if(byte == (RECEIVER_REPLY^receiver_ready)) {
            *state = StateBCC_RR;
        }
        else {
            *state = StateSTART;
        }
        break;
    case StateC_REJ:
        if(byte == FLAG){
            *state = StateFLAG;
        }
        else if(byte == (RECEIVER_REPLY^receiver_reject)) {
            *state = StateBCC_REJ;
        }
        else {
            *state = StateSTART;
        }
        break;
        
    case StateBCC_RR:
        if(byte == FLAG){
            *state = StateSTOP;
        }
        else {
            *state = StateSTART;
        }
        
        break;
    case StateBCC_REJ:
        *state = StateSTART;
        break;

    default:
        break;
    }
}

int transmitter_write(LinkLayer parameters, const unsigned char *buf, int bufSize) {
    
    (void)signal(SIGALRM, alarmHandler);
    (void)siginterrupt(SIGALRM,TRUE);

    unsigned char in_char;
    int ret;
    timeout_count = 0;
    alarm_enabled = FALSE;
    state = StateSTART;

    while (state != StateSTOP && timeout_count < parameters.nRetransmissions)
    {

        if (alarm_enabled == FALSE)
        {
            ret = write(fd, buf, bufSize);
            sleep(1);
            printf("Sent data package\n");
            alarm(parameters.timeout);
            state = StateSTART;
            alarm_enabled = TRUE;
        }

        read(fd, &in_char, 1);
        stateMachineTransmitter(&state, in_char);
    }

    alarm(0);

    if(timeout_count == parameters.nRetransmissions){
        printf("Max timeouts exceeded\n");
        exit(-1);
    }
    else{
        if (n_seq == 1){
            n_seq = 0;
        } else if (n_seq == 0){
            n_seq = 1;
        }
        printf("Received Receiver Ready frame\n");
    }
    return ret;
}
