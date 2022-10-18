// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include <signal.h>
#include <stdio.h>

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{

    LinkLayer connectionparameters;
    if (!strcmp(role, "tx")) {
        connectionparameters.role = LlTx;
    }
    else if (!strcmp(role, "rx")) {
        connectionparameters.role = LlRx;
    }
    else {
        printf("Error in role\n");
        exit(-1);
    }
    connectionparameters.baudRate = baudRate;
    strcpy(connectionparameters.serialPort, serialPort);
    connectionparameters.nRetransmissions = nTries;
    connectionparameters.timeout = timeout;

    int fd = llopen(connectionparameters);
}
