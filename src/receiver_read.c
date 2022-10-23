#include "receiver_read.h"

#define FLAG 0x7E
#define A 0x03
#define ESC 0x7D

extern int fd;
extern int n_seq;

State state;

void byteDestuffing (unsigned char * byte) {

    unsigned char result = byte ^ 0x20;

    byte = &result;

}

void stateMachineReceiver(State * state, unsigned char byte)
{
    unsigned char C;

    if (n_seq == 0) {
        C = 0x00;
    }
    else if (n_seq == 1) {
        C = 0x40;
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
        else {
            *state = StateSTART;
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

int receiver_read(unsigned char * packet) {

    unsigned char byte;
    unsigned char BCC2 = 0x00;
    unsigned char aux_packet[sizeof(packet)];
    int n_chars = 0;

    while(state != StateSTOP) {

        n_chars++;
        read(fd, &byte, 1);

        if (*state == StateDATA) {
            addToPacket(&aux_packet, byte);
            BCC2 = BCC2 ^ byte;
        }

        if (*state == StateDESTUFFING) {
            byteDestuffing(&byte);
        }

        stateMachineReceiver(&state, byte);

    }

    if (BCC2 == aux_packet[sizeof(aux_packet)]) {
        packet = &aux_packet;
        return n_chars;
    }
    else return -1;

}