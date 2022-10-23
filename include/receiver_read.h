// Receiver read header
#include "link_layer.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef _RECEIVER_READ_H
#define _RECEIVE_READ_H

typedef enum{
    StateSTART,
    StateFLAG,
    StateA,
    StateC,
    StateBCC1,
    StateDATA,
    StateDESTUFFING,
    StateSTOP
} State;

void stateMachine(State * state, unsigned char byte);

int receiver_read(unsigned char * packet);

unsigned char byteDestuffing(unsigned char byte);

#endif // _RECEIVER_READ_H
