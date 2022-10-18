// Frame Handler header.

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

#ifndef _FRAME_HANDLER_H
#define _FRAME_HANDLER_H

typedef enum{
    StateSTART,
    StateFLAG,
    StateA,
    StateC,
    StateBCC,
    StateSTOP
} State;

void stateMachine(State * state, unsigned char byte, LinkLayer connectionParameters);

int setupTermios(LinkLayer connectionParameters);

void alarmHandler(int signal);

int transmitter(int fd, unsigned char packet[], LinkLayer connectionParameters);

int receiver(int fd, unsigned char packet[], LinkLayer connectionParameters);

#endif // _FRAME_HANDLER_H
