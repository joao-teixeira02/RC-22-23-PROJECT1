// Write to serial port in non-canonical mode
//
// Modified by: Eduardo Nuno Almeida [enalmeida@fe.up.pt]

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1

#define BUF_SIZE 256
#define FLAG 0x7E
#define SET 0x03
#define UA 0x07
#define TRANSMITTER_COMMAND 0x03
#define RECEIVER_REPLY 0x03
#define RECEIVER_COMMAND 0x01
#define TRANSMITTER_REPLY 0x01
#define SU_BUF_SIZE 5

#define TRANSMITER 1
#define RECEIVER 0


#define TIMEOUT_SECS 3
#define MAX_TIMEOUTS 3



typedef enum{
    StateSTART,
    StateFLAG,
    StateA,
    StateC,
    StateBCC,
    StateSTOP
} State;

State state;

volatile int STOP = FALSE;
int alarm_enabled = FALSE;
int timeout_count = 0;
struct termios oldtio;
struct termios newtio;

void send_set(int fd){

    const unsigned char BCC1 = TRANSMITTER_COMMAND^SET;
    char su_buf[SU_BUF_SIZE] = {FLAG,TRANSMITTER_COMMAND,SET,BCC1,FLAG};
    write(fd, su_buf, SU_BUF_SIZE);
    printf("Sent set up frame\n");
    // Wait until all bytes have been written to the serial port
    sleep(1);

}

void receive_UA(State * state, unsigned char byte){
    
    unsigned char A = RECEIVER_REPLY;
    unsigned char C = UA;
    
    switch(*state){
    case StateSTART:
        if(byte == FLAG)
            *state = StateFLAG;
        
        break;
    case StateFLAG:
        if(byte == FLAG)
            *state = StateFLAG;
        else if(byte == A)
            *state = StateA;
        else
            *state = StateSTART;
        
        break;
    case StateA:
        if(byte == FLAG)
            *state = StateFLAG;
        else if(byte == C)
            *state = StateC;
        else
            *state = StateSTART;
        
        break;
    case StateC:
        if(byte == FLAG)
            *state = StateFLAG;
        else if(byte == (A^C))
            *state = StateBCC;
        else
            *state = StateSTART;
        
        break;
        
    case StateBCC:
        if(byte == FLAG)
            *state = StateSTOP;
        else
            *state = StateSTART;
        
        break;
    }

}


// Alarm function handler
void alarmHandler(int signal)
{
    alarm_enabled = FALSE;
    state = StateSTART;
    timeout_count++;
    printf("Timeout #%d\n", timeout_count);
}


void receive_set(State * state, unsigned char byte){
    
    unsigned char A = TRANSMITTER_COMMAND;
    unsigned char C = SET;
    
    switch(*state){
    case StateSTART:
        if(byte == FLAG)
            *state = StateFLAG;
        
        break;
    case StateFLAG:
        if(byte == FLAG)
            *state = StateFLAG;
        else if(byte == A)
            *state = StateA;
        else
            *state = StateSTART;
        
        break;
    case StateA:
        if(byte == FLAG)
            *state = StateFLAG;
        else if(byte == C)
            *state = StateC;
        else
            *state = StateSTART;
        
        break;
    case StateC:
        if(byte == FLAG)
            *state = StateFLAG;
        else if(byte == (A^C))
            *state = StateBCC;
        else
            *state = StateSTART;
        
        break;
        
    case StateBCC:
        if(byte == FLAG)
            *state = StateSTOP;
        else
            *state = StateSTART;
        
        break;
    }

}

int llopen(const char * port, int flag){
    // Open serial port device for reading and writing and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    
    state = StateSTART;

    int fd = open(port, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        perror(port);
        exit(-1);
    }


    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        exit(-1);
    }

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0; // Inter-character timer unused
    newtio.c_cc[VMIN] = 1;  // Blocking read until 1 chars received

    // VTIME e VMIN should be changed in order to protect with a
    // timeout the reception of the following character(s)

    // Now clean the line and activate the settings for the port
    // tcflush() discards data written to the object referred to
    // by fd but not transmitted, or data received but not read,
    // depending on the value of queue_selector:
    //   TCIFLUSH - flushes data received but not read.
    tcflush(fd, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");


    unsigned char in_char;
    

    if(flag == TRANSMITER){
        (void)signal(SIGALRM, alarmHandler);
        (void)siginterrupt(SIGALRM,TRUE); //system call interrupted by alarm isn't restarted
        // Loop for input
        unsigned char in_char;
        
        while (state != StateSTOP && timeout_count < MAX_TIMEOUTS)
        {

            if (alarm_enabled == FALSE)
            {
                send_set(fd);
                alarm(TIMEOUT_SECS); // Set alarm
                alarm_enabled = TRUE;
            }

            // Returns after 1 chars has been input
            read(fd, &in_char, 1);        
            receive_UA(&state,in_char);

        }

        alarm(0);

        if(timeout_count == MAX_TIMEOUTS){
            printf("Max timeouts exceeded\n");
        }
        else{
            printf("Received unnumbered acknowledgment frame\n");
            printf("Connection established\n");
        }

    }else{

        while (state != StateSTOP)
        {
            // Returns after 1 chars has been input
            read(fd, &in_char, 1);        
            receive_set(&state, in_char);
        }
        
        printf("Received set up frame\n");
        
        unsigned char bcc = RECEIVER_REPLY^UA;
        unsigned char su_buf[SU_BUF_SIZE] = {FLAG, RECEIVER_REPLY, UA, bcc, FLAG};
        
        write(fd, su_buf, SU_BUF_SIZE);
        printf("Sent unnumbered acknowledgment frame\n");

        // Wait until all bytes have been written to the serial port
        sleep(1);
        printf("Connection established\n");
    }
    return fd;
}


int main(int argc, char *argv[])
{
    // Program usage: Uses either COM1 or COM2
    const char *serialPortName = argv[1];
    const char *flag = argv[2];


    if (argc < 3)
    {
        printf("Incorrect program usage\n"
               "Usage: %s <SerialPort> <TRANSMITER/RECEIVER>\n"
               "Example: %s /dev/ttyS1 TRANSMITER \n",
               argv[0],
               argv[0]);
        exit(1);
    }
    int fd = -1;

    // Connect

    if(strncmp(argv[2], "TRANSMITER", 20) == 0){

    fd = llopen(serialPortName, TRANSMITER);
    
    }else if(strncmp(argv[2], "RECEIVER", 20) == 0){

        fd = llopen(serialPortName, RECEIVER);
    }else{
        printf("Incorrect program usage\n"
               "Usage: %s <SerialPort> <TRANSMITER/RECEIVER>\n"
               "Example: %s /dev/ttyS1 TRANSMITER \n",
               argv[0],
               argv[0]);
        exit(1);
    }
    
    // Send I(0)



    // Disconnect

    // Restore the old port settings
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);

    return 0;
}