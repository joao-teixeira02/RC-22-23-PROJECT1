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
#define L2 0x02
#define L1 0x00

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

    FILE *in;
    in = fopen(filename, "rb");

    fseek(in, 0L, SEEK_END);
    long int sz = ftell(in);
    fseek(in, 0L, SEEK_SET);

    fclose(in);

    int size_counter = 1;
    long int size = sz;
    while(size >>= 1){
        size_counter++;
    }

    char byte_size = 0x0;
    char size_in_bytes[size_counter];

    unsigned char control_packet[7];

    control_packet[0] = CONTROL_START;
    control_packet[1] = TYPE_SIZE;
    control_packet[2] = byte_size;

    int x = 0;

    for (int i = 3; i < 7; i++) {
        control_packet[i] = size_in_bytes[x];
        x++;
    }

    llwrite(control_packet, 7);

    unsigned char info_packet[MAX_PAYLOAD_SIZE];
    info_packet[0] = CONTROL_DATA;

    char c = fgetc(in);
    int char_counter = 0;
    int n_seq = 0;
    char hex_n_seq;

    while (!feof(in))
    {
        info_packet[1] = sprintf(hex_n_seq, "%x", n_seq%255);
        info_packet[2] = L2;
        info_packet[3] = L1;
        while (char_counter < 996) {
            if (!feof(in)) break;
            info_packet[4+char_counter] = c;
            printf("info_packet value %d: %c\n", 4+char_counter, info_packet[4+char_counter]);
            char_counter++;
            c = fgetc(in);
        }
        llwrite(info_packet, MAX_PAYLOAD_SIZE);
        n_seq++;
    }

    fclose(in);

}
