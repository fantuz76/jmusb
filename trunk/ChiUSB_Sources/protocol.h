#ifndef __PROTOCOLH
#define __PROTOCOLH


#define REQ_HELLO         0x21
#define REQ_LIST_INTERV   0x31
#define REQ_ERR_0         0x41



#define INTERVENTO_LENGTH 24

// Lunghezza Massima in byte dati su USB (compresi checksum, length etc..)
#define MAX_LEN_CMD       40
#define MAX_LEN_PAYLOAD   MAX_LEN_CMD-3

#define FIRST_BYTE_EMPTY  0x80


typedef union {
  byte BYTE;
  struct {
    byte B0 :1;  
    byte B1 :1;  
    byte B2 :1;  
    byte B3 :1;  
    byte B4 :1;  
    byte B5 :1;  
    byte B6 :1;  
    byte B7 :1;  
  } BIT;
} byte_def;
  


void comm_init(int (*putch_)(char), int (*getch_)(void), int(*kbhit_)(void));
void comm_process(void);


#endif