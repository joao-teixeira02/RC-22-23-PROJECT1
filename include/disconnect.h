// Disconnect read header
#include "link_layer.h"
#include "frame_handler.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef _DISCONNECT_H
#define _DISCONNECT_H

void stateMachineDisc(State * state, unsigned char byte, LinkLayer connectionParameters);

int transmitterDisc(unsigned char packet[], LinkLayer connectionParameters);

int receiverDisc(unsigned char packet[], LinkLayer connectionParameters);

#endif // _DISCONNECT_H
