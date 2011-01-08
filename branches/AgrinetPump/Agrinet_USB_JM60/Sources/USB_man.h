#ifndef __USB_MANH
#define __USB_MANH

#include "usb.h"
#include "target.h"
#include "usb_cdc.h"



#define REQ_HELLO         0x21
#define REQ_LIST_INTERV   0x31
#define REQ_ERR_0         0x41


//#define NUM_MAX_INTERV    505
#define INTERVENTO_LENGTH 16

// Lunghezza Massima in byte dati su USB (compresi checksum, length etc..)
#define MAX_LEN_CMD       28
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
  



void USB_comm_init(void);
void USB_comm_process(void);
void USB_time_sw(void);

void Init_USB_man(void);

#endif