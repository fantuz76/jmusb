#include "usb_cdc.h" 
#include "terminal/hcc_terminal.h"
#include "protocol.h"
#include <stdtypes.h>


/*****************************************************************************
 * Module variables.
 *****************************************************************************/

static byte usbNumByteLen;
static byte usbPayloadLen;

static char usbrxar[MAX_LEN_CMD];
//static char *usbrxar;
static byte usbrxar_ndx;


byte_def _USB_flags1;
#define USB_flags1    _USB_flags1.BYTE
#define USBPktReady   _USB_flags1.BIT.B0
//#define xxx    _USB_flags1.BIT.B1

//static byte n_cmd;
//static command_t * cmds[MAX_CMDS];


static int (*putch)(char);
static int (*getch)(void);
static int (*kbhit)(void);


void ResetPkt_ndx(void){
  usbrxar_ndx = 0;
  usbPayloadLen = 0;
  usbNumByteLen = 0;        
  USBPktReady = FALSE;
}


/*****************************************************************************
 * Name:
 *    print
 * In:
 *    s: string
 * Out:
 *    n/a
 *
 * Description:
 *    Print the specified string.
 * Assumptions:
 *
 *****************************************************************************/
void print(char *s)
{
  while(*s)
  {
    while(*s != (char)putch(*s))
      ;
    s++;
  }
}


/*****************************************************************************
* Name:
*    comm_init
* In:
*    N/A
*
* Out:
*    N/A
*
* Description:
*    Inicialise the terminal. Set local varaibles to a default value, print
*    greeting message and prompt.
*
* Assumptions:
*    --
*****************************************************************************/
void comm_init(int (*putch_)(char), int (*getch_)(void), int(*kbhit_)(void))
{
  usbrxar[sizeof(usbrxar)-1]='\0';
  
  
  ResetPkt_ndx();

  //cmds[0]=(void *)&help_cmd;
  //n_cmd=1;
  
  putch=putch_;
  getch=getch_;
  kbhit=kbhit_;

  //print_greeting();
  //print_prompt();
}                                



/*****************************************************************************
* Name:
*    ParseFirstByte
* In:
*    N/A
*
* Out:
*    N/A
*
* Description:
*    
*
* Assumptions:
*    --
*****************************************************************************/
byte ParseFirstByte(byte _byteToParse) 
{
  if ((_byteToParse & 0xF0) == FIRST_BYTE_EMPTY) {    
    usbNumByteLen = (_byteToParse & 0x03);
  } else {
    // Error - pkt unknown
    return(0);        
  }  
}




/*****************************************************************************
 * Name:
 *    USB_PktSend
 * In:
 *    _toSend: 
 * Out:
 *    n/a
 *
 * Description:
 *    Print the specified string.
 * Assumptions:
 *
 *****************************************************************************/
void USB_PktSend(byte *_toSend, byte _len)
{
  byte cnt=0, byteToSend, cks=0;
  
  byteToSend = FIRST_BYTE_EMPTY;  
  if (_len>0) byteToSend |= 0x01;
  
  
  // Primo Byte
  while(byteToSend != (char)cdc_putch(byteToSend))
    ;  
  cks += byteToSend;

  // Byte Length
  if (_len>0){    
    byteToSend = _len;
    while(byteToSend != (char)cdc_putch(byteToSend))
      ;
    cks += byteToSend;
  }
  
  
  // PayLoad
  while (cnt<_len){  
    while(*_toSend != (char)cdc_putch(*_toSend))
      ;
    cks += (*_toSend);
    _toSend++;
    cnt++;
  }
  
  
  // CheckSum
  while(cks != (char)cdc_putch(cks))
    ;
  
}
   
 
 
   
                      
/*****************************************************************************
* Name:
*    comm_proces
* In:
*    N/A
*
* Out:
*    N/A
*
* Description:
*    
*
* Assumptions:
*    --
*****************************************************************************/
void comm_process(void)
{

  byte USBtoSend[MAX_LEN_PAYLOAD];
  byte i;
  byte cks;
  // finchè c'è qualcosa nel buffer USB
  // Se ci sono più di 32 byte entro + volte qua
  
  while((cdc_kbhit)())
  {
      
    char c;
    c=(char)(cdc_getch)();        
    usbrxar[usbrxar_ndx++]=c;
    
    if (usbrxar_ndx >= MAX_LEN_CMD)   // Overflow
      ResetPkt_ndx();
    
   
    
    if (usbrxar_ndx-1==0) {    
      // primo byte rx
      if ((usbrxar[usbrxar_ndx-1] & 0xF0) == FIRST_BYTE_EMPTY) {        
        // C'è la Lunghezza?  
        usbNumByteLen = (usbrxar[usbrxar_ndx-1] & 0x01);        
                    
      } else {
        // Error - pkt unknown
        ResetPkt_ndx();
      }      

    } else if ( (usbNumByteLen==1) &&  (usbrxar_ndx-1==1)) {
      // Sono in lunghezza      
        usbPayloadLen = usbrxar[1];         
    
    } else if ((usbrxar_ndx-1 >= usbNumByteLen+1)  && (usbrxar_ndx-1 < usbPayloadLen+usbNumByteLen+1)) {
      // Sono in PayLoad
            
      
    } else if (usbrxar_ndx-1 == usbPayloadLen+usbNumByteLen+1) {
      // Sono in ChkSum
      for (i=0,cks=0; i<usbrxar_ndx-1; cks += usbrxar[i++]);
              
      if (cks != usbrxar[i])
      {
        ResetPkt_ndx();
      } else {
        USBPktReady = TRUE;        
      }
      
    
    } else {
      // altro?
      ResetPkt_ndx();
      
    }
    
    
    
    
    if (USBPktReady) {
      
      // Analizzare messaggio ricevuto
      
      if (usbPayloadLen==0) {      
        USB_PktSend(USBtoSend,0); 
      } else {
        switch (usbrxar[usbNumByteLen+1]) {
          case 0x33:
            USBtoSend[0] = 0x30;
            USBtoSend[1] = 0x31;
            USBtoSend[2] = 0x32;
            USBtoSend[3] = 0x33;
            
            USB_PktSend(USBtoSend,3);          
          break;


          case REQ_ERR_0:
            
            USBtoSend[0] = 0x40;
            USBtoSend[1] = 0x40;
            if (SW1_ACTIVE()) {
              USBtoSend[0] |= 0x05;
              
            }

            if (SW2_ACTIVE()) {
              USBtoSend[1] |= 0x07;
              
            }
          
            
            USBtoSend[2] = 0x42;
            USBtoSend[3] = 0x43;
            
            USB_PktSend(USBtoSend,4);          
          break;

          default:
            USBtoSend[0] = 0x55;
            USB_PktSend(USBtoSend,1);
          break;          
        }
      
      }
      

      ResetPkt_ndx();      
      
    }
    
    
    
    
    
    // echo
    //while(c!=(char)(putch)(c))
    //  ;
      
    //if (c=='\r')
    //{
    //  while('\n'!=(char)(putch)('\n'))
    //    ;
    //}
    // Execute command if enter is received, or usbrxar is full./
   /* if ((c=='\r') || (usbrxar_ndx == sizeof(usbrxar)-2))
    {
      //int start=skipp_space(usbrxar, 0);
      //int end=find_word(usbrxar, start);
      int x=0;

      // Separate command string from parameters, and close parameters string.
      //usbrxar[end]=usbrxar[usbrxar_ndx]='\0';

      // Identify command. 
      //x=find_command(usbrxar+start);
      // Command not found. 

     // if (x == -1)
      {
     //   print("Unknown command!\r\n");
      }
     // else
      {
        //(*cmds[x]->func)(usbrxar+end+1);
      }
      usbrxar_ndx=0;
//      print_prompt();
    }
    else
    { // Put character to usbrxar.
      if (c=='\b')
      {
        if (usbrxar_ndx > 0)
        {
          usbrxar[usbrxar_ndx]='\0';
          usbrxar_ndx--;
        }
      }
      else
      {
        usbrxar[usbrxar_ndx++]=c;
      }
    }        
         */  
  }
}
