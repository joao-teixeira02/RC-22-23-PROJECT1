// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include <signal.h>
#include <stdio.h>

#define CONTROL_DATA 0x01
#define CONTROL_START 0x02
#define CONTROL_END 0x03
#define TYPE_SIZE 0x00

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

    //int fd = llopen(connectionparameters);

    FILE *in, *out;
    in = fopen(filename, "rb");
    //out = fopen("penguin_mod.gif", "wb");

    /*char c = fgetc(in);
    while (!feof(in))
    {
        fputc(c, out);
        c = fgetc(in);
    }

    fclose(in);
    fclose(out);*/

    fseek(in, 0L, SEEK_END);
    int sz = ftell(in);
    fseek(in, 0L, SEEK_SET);

    printf("Tamanho do ficheiro: %d\n", sz);

    char control_packet[4];
    control_packet[0] = CONTROL_START;
    control_packet[1] = TYPE_SIZE;
    //control_packet[2] = 


}
