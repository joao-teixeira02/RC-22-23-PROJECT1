#include "receiver_read.h"

#define FLAG 0x7E
#define A 0x03
#define ESC 0x7D

extern int fd;
extern int n_seq;

State state;

void byteDestuffing (unsigned char * byte) {

    unsigned char result = *byte ^ 0x20;

    byte = &result;

}

void stateMachineReceiver(State * state, unsigned char byte)
{
    unsigned char C;
    unsigned char C_REPLY;

    if (n_seq == 0) {
        C = 0x00;
        C_REPLY = 0x40;
    }
    else if (n_seq == 1) {
        C = 0x40;
        C_REPLY = 0x00;
    }
    else {
        printf("Error in n_seq -> not a valid value (0 or 1)\n");
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
        else if(byte == C_REPLY) {
            *state = StateREPLY;
        }
        else {
            *state = StateSTART;
        }
        break;

    case StateREPLY:
        if (byte == FLAG) {
            *state =  StateSTOP_REPLY;
        }
        break;

    case StateC:

        if(byte == FLAG){
            *state = StateFLAG;
        }
        else if(byte == (A^C)) {
            *state = StateBCC1;
        }
        else {
            *state = StateSTART;
        }
        break;
        
    case StateBCC1:

        if(byte == FLAG){
            *state = StateSTART;
        }
        else {
            *state = StateDATA;
        }   
        break; 

    case StateDATA:

        if(byte == ESC) {
            *state = StateDESTUFFING;
        }
        else if(byte == FLAG) {
            *state = StateSTOP;
        }
        else {
            *state = StateDATA;
        }
        break;

    case StateDESTUFFING:
        *state = StateDATA;
        break;
    }
}

int receiver_write(unsigned char * packet, int size) {
    write(fd, packet, size);
}

int receiver_read(unsigned char * packet) {

    unsigned char byte;
    unsigned char BCC2 = 0x00;
    int n_chars = 0;

    state = StateSTART;

    while(state != StateSTOP) {

        if (state == StateSTOP_REPLY) {
            return -2;
        }

        read(fd, &byte, 1);

        if (state == StateDATA) {
            packet[n_chars] = byte;
            n_chars++;
            BCC2 = BCC2 ^ byte;
        }
        if (state == StateDESTUFFING) {
            byteDestuffing(&byte);
        }

        stateMachineReceiver(&state, byte);

    }

    for (int i = 0; i < n_chars; i++) {
        printf("packet[%d] = %x\n", i, packet[i]);
    }

    printf("packet[n_chars-2] = %x\n", packet[n_chars-2]);
    printf("BCC2 = %x\n", BCC2);

    if (BCC2 == packet[n_chars-2]) {
        return n_chars;
    }
    else return -1;

}