// Receiver read header
#include "link_layer.h"
#include "frame_handler.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "macros.h"

#ifndef _RECEIVER_READ_H
#define _RECEIVE_READ_H

void stateMachineReceiver(State * state, unsigned char byte);

int receiver_write(unsigned char * packet, int size);

int receiver_read(unsigned char * packet);

#endif // _RECEIVER_READ_H
