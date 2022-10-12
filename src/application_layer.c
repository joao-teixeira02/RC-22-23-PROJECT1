// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include <signal.h>

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{

    LinkLayer connectionparameters;
    connectionparameters.role = role;
    connectionparameters.baudRate = baudRate;
    *connectionparameters.serialPort = serialPort; 
    connectionparameters.nRetransmissions = nTries;
    connectionparameters.timeout = timeout;

    int fd = llopen(connectionparameters);

    unsigned char in_char[] = {0};
    unsigned char out_char[] = {0};
}
