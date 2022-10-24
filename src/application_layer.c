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

    if (connectionparameters.role == LlTx) {
        
        FILE *in;
        in = fopen(filename, "rb");

        fseek(in, 0L, SEEK_END);
        long int file_size = ftell(in);
        fseek(in, 0L, SEEK_SET);

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

        llwrite(control_packet, 3 + size_in_bytes);

        unsigned char info_packet[994];
        info_packet[0] = CONTROL_DATA;

        char c = fgetc(in);
        int char_counter = 0;
        int n_seq = 0;
        int stop = 0;
        while (!stop)
        {
            info_packet[1] = n_seq%255;
            while (char_counter < 990 && !stop) {
                info_packet[4 + char_counter] = c;
                char_counter++;
                c = fgetc(in);
                if (c == EOF) {
                    if (feof(in)) {
                        stop = 1;
                    }
                }
            }
            info_packet[2] = char_counter / 256;
            info_packet[3] = char_counter % 256;
            llwrite(info_packet, 4 + char_counter);
            n_seq++;
            char_counter = 0;
        }

        control_packet[0] = CONTROL_END;
        llwrite(control_packet, 3 + size_in_bytes);

        fclose(in);

    } else if (connectionparameters.role == LlRx){

        unsigned char packet[11];

        int n_chars = llread(&packet);

        for(int i = 0; i < n_chars; i++) {
            printf("info_packet value %d: %x\n", i, packet[i]);
        }
/* 
        unsigned char in_char;

        //Reading Control packet
        for(int x = 0; x < 4; x++){
            llread(&in_char);
        }

        File *out;
        out = fopen(filename, "wb");

        //Reading Info packets
        while (state != stateSTOP)
        {
            llread(&in_char);
            //statemachine(&in_char);
        }
        
        fclose(out); */
    }   

}
