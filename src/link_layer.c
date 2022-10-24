// Link layer protocol implementation

#include "link_layer.h"
#include "frame_handler.h"
#include "receiver_read.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>


// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1

#define BUF_SIZE 256
#define FLAG 0x7E
#define TRANSMITTER_COMMAND 0x03
#define TRANSMITTER_REPLY 0x01
#define RECEIVER_COMMAND 0x01
#define RECEIVER_REPLY 0x03
#define CONTROL_SET 0x03
#define CONTROL_UA 0x07
#define BCC_HEADER 0x01
#define BCC_DATA 0x02
#define CONTROL_DATA 0x01
#define CONTROL_START 0x02
#define CONTROL_END 0x03
#define SU_BUF_SIZE 5
#define ESCAPE 0x7D
#define FLAG_SUBST 0x5E
#define ESCAPE_SUBST 0x5D

#define TIMEOUT_SECS 3
#define MAX_TIMEOUTS 3

int fd;
int n_seq = 0;
int n_res = 1;
LinkLayer parameters;
extern int timeout_count;
extern int alarm_enabled;

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
    parameters = connectionParameters;
    fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        perror(connectionParameters.serialPort);
        exit(-1);
    }

    setupTermios(connectionParameters);
    
    if (connectionParameters.role == LlTx) {

        unsigned char SET_packet[SU_BUF_SIZE];

        SET_packet[0] = FLAG;
        SET_packet[1] = TRANSMITTER_COMMAND;
        SET_packet[2] = CONTROL_SET;
        SET_packet[3] = (TRANSMITTER_COMMAND ^ CONTROL_SET);
        SET_packet[4] = FLAG;

        transmitter(SET_packet, connectionParameters);
    }
    else if (connectionParameters.role == LlRx) {

        unsigned char UA_packet[SU_BUF_SIZE];
        
        UA_packet[0] = FLAG;
        UA_packet[1] = RECEIVER_REPLY;
        UA_packet[2] = CONTROL_UA;
        UA_packet[3] = (RECEIVER_REPLY^CONTROL_UA);
        UA_packet[4] = FLAG;

        receiver(UA_packet, connectionParameters);
    }
    else {
        printf("Error in llopen: %d is an invalid value for role", connectionParameters.role);
        exit(-1);
    }

    return fd;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{

    /*
    - -> recebe control e info packets do app layer
    - -> faz byte stuffing 
    - -> cria uma trama a partir deles 
    - -> escreve a trama para o receiver
    - -> esperar RR para terminar
    - -> (usa state machine para verificar valores e auxilio no byte stuffing)
    */
    unsigned char data_package[2000];
    data_package[0] = FLAG;
    data_package[1] = TRANSMITTER_COMMAND;
    data_package[2] = (Ns) << 6;
    data_package[4 + bufSize + 1] = FLAG;

    if(buf[0] == CONTROL_START || buf[0] == CONTROL_END){
        data_package[3] = BCC_HEADER;
        data_package[4 + bufSize] = BCC_HEADER;
    } else if (buf[0] == CONTROL_DATA) {
        data_package[3] = BCC_DATA;
        data_package[4 + bufSize] = BCC_DATA;
    } else {
        printf("Error reading buf[0] value in llwrite()\n");
    }

    int offset = 0;
    for(int x = 0; x < bufSize; x++){
        if (buf[x] == FLAG) {
            data_package[4 + x + offset] = ESCAPE;
            data_package[4 + x + 1 + offset] = FLAG_SUBST;
            offset++;
        } else if (buf[x] == ESCAPE) {
            data_package[4 + x + offset] = ESCAPE;
            data_package[4 + x + 1 + offset] = ESCAPE_SUBST;
            offset++;
        } else {
            data_package[4 + x + offset] = buf[x];       
        }
    }

    (void)signal(SIGALRM, alarmHandler);
    (void)siginterrupt(SIGALRM,TRUE);

    unsigned char in_char;
    State state;
    int ret;
    timeout_count = 0;
    alarm_enabled = FALSE;

    while (state != StateSTOP && timeout_count < parameters.nRetransmissions)
    {

        if (alarm_enabled == FALSE)
        {
            ret = write(fd, data_package, 4 + bufSize + 2 + offset);
            sleep(1);
            printf("Sent data package\n");
            alarm(parameters.timeout);
            state = StateSTART;
            alarm_enabled = TRUE;
        }

        read(fd, &in_char, 1);
        stateMachine_Transmitter(&state, in_char);
    }

    alarm(0);

    if(timeout_count == parameters.nRetransmissions){
        printf("Max timeouts exceeded\n");
        exit(-1);
    }
    else{
        printf("Received Receiver Ready frame\n");
        printf("Connection established\n");
    }
    return ret;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    /*
    - recebe as tramas
    - desfazer byte stuffing
    - usa state machine para verificar se os valores que está a receber estão corretos
    - guarda o packet recebido dentro da trama no packet de argumento
    - envia RR como confirmação
    */

    unsigned char RR_packet[SU_BUF_SIZE];
    unsigned char REJ_packet[SU_BUF_SIZE];

    REJ_packet[0] = FLAG;
    REJ_packet[1] = RECEIVER_REPLY;

    RR_packet[0] = FLAG;
    RR_packet[1] = RECEIVER_REPLY;

    if (n_res == 1) {
        RR_packet[2] = 0x85;
        REJ_packet[2] = 0x81;
    }
    else if (n_res == 0) {
        RR_packet[2] = 0x05;
        REJ_packet[2] = 0x01;
    }
    else {
        printf("Error in n_res -> not a valid value (0 or 1)\n");
        exit(-1);
    }

    REJ_packet[3] = RECEIVER_REPLY ^ RR_packet[2];
    REJ_packet[4] = FLAG;

    RR_packet[3] = RECEIVER_REPLY  ^ RR_packet[2];
    RR_packet[4] = FLAG;

    int n;

    do {
        n = receiver_read(packet);
        if (n == -2) receiver_write(RR_packet, SU_BUF_SIZE);
        if (n == -1) receiver_write(REJ_packet, SU_BUF_SIZE);
    } while (n <= 0);

    receiver_write(RR_packet, SU_BUF_SIZE);

    printf("Number of chars in llread: %d\n", n);
        
    return n;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    /*if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    return close(fd); */
    return 0;
}
