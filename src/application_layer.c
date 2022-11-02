// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <macros.h>
#include <time.h>

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

    clock_t start_t, end_t;
    double total_t;

    llopen(connectionparameters);

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

        unsigned char info_packet[999];
        info_packet[0] = CONTROL_DATA;

        char c = fgetc(in);
        int char_counter = 0;
        int n_seq = 0;
        int stop = 0;

        start_t = clock();

        sleep(10);

        while (!stop)
        {
            info_packet[1] = n_seq%255;
            while (char_counter < 995 && !stop) {
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

        end_t = clock();

        total_t = (double)(end_t - start_t) / CLOCKS_PER_SEC;

        double theor_time = (file_size*8)/baudRate; //theoretical time

        printf("Total time to send all info packets: %f ms\n", total_t*1000);

        printf("Efficiency: %f\n", theor_time/total_t);

        control_packet[0] = CONTROL_END;
        llwrite(control_packet, 3 + size_in_bytes);

        fclose(in);

    } else if (connectionparameters.role == LlRx){

        FILE *out;
        out = fopen("penguin_new.gif", "wb");

        unsigned char packet[1000];

        int n_chars = llread(&packet);

        while (packet[0] != 3) {
            n_chars = llread(&packet);
            printf("Read %d chars\n", n_chars);
            if (packet[0] == 1) {
                for (int i = 4; i < packet[2]*256+packet[3]+4; i++) {
                    fputc(packet[i], out);
                }
            }
        }

        fclose(out);
    }

    llclose(0);

}
