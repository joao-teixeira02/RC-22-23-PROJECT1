// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include <signal.h>
#include <stdio.h>
#include <math.h>

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
    fclose(out); */

    fseek(in, 0L, SEEK_END);
    long int sz = ftell(in);
    fseek(in, 0L, SEEK_SET);

    fclose(in);

    int size_counter = 1;
    long int size = sz;
    while(size >>= 1){
        size_counter++;
    }

    char byte_size;
    char size_in_bytes[size_counter];

    double result1;

    result1 = (double) size_counter / 8.0;

    double result = ceil(result);

    printf("Result: %f\n", result);

    printf("Tamanho do ficheiro: %ld bytes\n", sz);
    printf("Tamanho do Tamanho do fichero: %d bits\n", size_counter);

    sprintf(&byte_size, "%x", result);
    printf("Byte size: %c\n", byte_size);
    sprintf(&size_in_bytes, "%x", sz);
    printf("size in bytes: %s\n", size_in_bytes);

    char control_packet[7];

    control_packet[0] = CONTROL_START;
    control_packet[1] = TYPE_SIZE;
    control_packet[2] = byte_size;

    int x = 0;

    for (int i = 3; i < 7; i++) {
        control_packet[i] = size_in_bytes[x];
        x++;
    }

    printf("%x\n", control_packet[0]);
    printf("%x\n", control_packet[1]);
    printf("%x\n", control_packet[2]);
    printf("%x\n", control_packet[3]);
    printf("%x\n", control_packet[4]);
    printf("%x\n", control_packet[5]);
    printf("%x\n", control_packet[6]);

}
