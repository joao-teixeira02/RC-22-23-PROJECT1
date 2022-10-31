// Application layer protocol implementation

#include "application_layer.h"

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

    llopen(connectionparameters);

    if (connectionparameters.role == LlTx) {
        
        FILE *in;
        in = fopen(filename, "rb");

        fseek(in, 0L, SEEK_END);
        long int file_size = ftell(in);
        fseek(in, 0L, SEEK_SET);

        unsigned char control_packet[3 + file_size];

        control_packet[0] = CONTROL_START;
        control_packet[1] = TYPE_SIZE;
        control_packet[2] = file_size;

        for (int i = 0; i < file_size; i++) {
            control_packet[3 + file_size - 1 - i] = (file_size & (0xff << (8 * i))) >> (8 * i);
        }

        llwrite(control_packet, 3 + file_size);

        unsigned char info_packet[999];
        info_packet[0] = CONTROL_DATA;

        char c = fgetc(in);
        int char_counter = 0;
        int n_seq = 0;
        int stop = 0;
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

        control_packet[0] = CONTROL_END;
        llwrite(control_packet, 3 + file_size);

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
