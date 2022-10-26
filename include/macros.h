#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1

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
#define CONTROL_DISC 0x0B
#define RECEIVER_READY 0x05
#define CONTROL_DISC 0x0B
#define TIMEOUT_SECS 3
#define MAX_TIMEOUTS 3
#define TYPE_SIZE 0x00
#define L2 0x02
#define L1 0x00