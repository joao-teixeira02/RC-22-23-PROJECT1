// Transmitter write header

#include "link_layer.h"
#include "frame_handler.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "macros.h"

#ifndef _TRANSMITTER_WRITE_H
#define _TRANSMITTER_WRITE_H

void stateMachineTransmitter(State * state, unsigned char byte);

int transmitter_write(LinkLayer parameters, const unsigned char *buf, int bufSize);

#endif // _TRANSMITTER_WRITE_H
