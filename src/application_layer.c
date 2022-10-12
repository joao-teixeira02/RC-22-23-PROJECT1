// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include <signal.h>

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{

    LinkLayer connectionparameters;
    printf("role :%s\n", role);
    if (!strcmp(role, "tx")) {
        connectionparameters.role = 0;
    }
    else if (!strcmp(role, "rx")) {
        connectionparameters.role = 1;
    }
    else {
        printf("Error in role\n");
        exit(-1);
    }
    connectionparameters.baudRate = baudRate;
    *connectionparameters.serialPort = serialPort; 
    connectionparameters.nRetransmissions = nTries;
    connectionparameters.timeout = timeout;

    printf("filename: %s\n", filename);

    int fd = llopen(connectionparameters);

    unsigned char in_char[] = {0};
    unsigned char out_char[] = {0};
}
