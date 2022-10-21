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
    long int file_size = ftell(in);
    fseek(in, 0L, SEEK_SET);

    fclose(in);

    int size_in_bits = 1;
    long int temp = file_size;
    while(temp >>= 1){
        size_in_bits++;
    }

    int size_in_bytes = (size_in_bits + (8 - 1)) / 8;

    unsigned char control_packet[3 + size_in_bytes];

    control_packet[0] = CONTROL_START;
    control_packet[1] = TYPE_SIZE;
    control_packet[2] = size_in_bytes;

    for (int i = 0; i < size_in_bytes; i++) {
        control_packet[3 + size_in_bytes - 1 - i] = (file_size & (0xff << (8 * i))) >> (8 * i);
    }

/*     printf("Pacote de Controlo \n");
    printf("Campo de Controlo: %x\n", control_packet[0]);
    printf("Tipo de parametro: %x\n", control_packet[1]);
    printf("Tamanho do parametro: %d bytes\n", control_packet[2]);

    for(int x = 0; x < size_in_bytes; x++){
        printf("Tamanho do ficheiro %d: %x\n", x, control_packet[3 + x]);
    } */

    


}
