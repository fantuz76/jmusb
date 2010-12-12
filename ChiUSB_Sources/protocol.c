#include "usb_cdc.h" 
#include "terminal/hcc_terminal.h"
#include "protocol.h"
#include <stdtypes.h>
#include <utils.h>

//#include <stdlib.h>



/*****************************************************************************
 * Module variables.
 *****************************************************************************/

static byte usbNumByteLen;
static byte usbPayloadLen;

static char usbrxar[MAX_LEN_CMD];
//static char *usbrxar;
static byte usbrxar_ndx;


static byte arrIntervento[INTERVENTO_LENGTH];

static byte_def _USB_flags1;
#define USB_flags1    _USB_flags1.BYTE
#define USBPktReady   _USB_flags1.BIT.B0
//#define xxx    _USB_flags1.BIT.B1

//static byte n_cmd;
//static command_t * cmds[MAX_CMDS];




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
    while(*s != (char)cdc_putch(*s))
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
void comm_init(void)
{

  usbrxar[sizeof(usbrxar)-1]='\0';
  
  
  ResetPkt_ndx();

  //cmds[0]=(void *)&help_cmd;
  //n_cmd=1;
  
  //putch=putch_;
  //getch=getch_;
  //kbhit=kbhit_;

  //print_greeting();
  //print_prompt();
  
  
  
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
 *    Send Pkt
 *    FIRST_BYTE + (LENGTH) + PAYLOAD + CHKSUM
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
 *    USB_SendHello
 * In:
 *    _toSend: 
 * Out:
 *    n/a
 *
 * Description:
 *      Send Hello
 * Assumptions:
 *
 *****************************************************************************/
void USB_SendHello(void)
{
  byte _myarr[MAX_LEN_PAYLOAD];

  _myarr[0] = REQ_HELLO;  // Indica risposta ad Hello
  
  
  // Invia matricola e ore lavoro e...
  _myarr[1] = 0x23;
  _myarr[2] = 0x45;
  _myarr[3] = 0x67;    
  _myarr[4] = 0x55;
  
  _myarr[5] = 0x55;
  _myarr[6] = 0x55;
  _myarr[7] = 0x55;
  _myarr[7] = 0x44;
  USB_PktSend(_myarr,9);   

}
    
 



byte *ReadIntervento(word _PosMem) {

  arrIntervento[0] = 16;
  
  
  
  arrIntervento[1] = 0;
  arrIntervento[2] = 0;
  arrIntervento[3] = _PosMem;
  arrIntervento[4] = _PosMem;
  
  
  arrIntervento[5] = 0x01;
  arrIntervento[6] = 0x2C;

  arrIntervento[7] = 0x01;
  arrIntervento[8] = 0x2D;

  arrIntervento[9] = 0x01;
  arrIntervento[10] = 0x2E;

  arrIntervento[11] = 0x1B;
  arrIntervento[12] = 0x58;

  arrIntervento[13] = 0x1B;
  arrIntervento[14] = 0x59;

  arrIntervento[15] = 0x1B;
  arrIntervento[16] = 0x5A;


  arrIntervento[17] = 0x0F;
  arrIntervento[18] = 0x44;


  arrIntervento[19] = 0x04;
  arrIntervento[20] = 0xCD;
  
  
  arrIntervento[21] = 0x62;
  
  arrIntervento[22] = 0x00;
  arrIntervento[23] = 0x66;

    
  return(arrIntervento);  
  
}
   


/*****************************************************************************
 * Name:
 *    USB_SendTuttiInterventi
 * In:
 *    _toSend: 
 * Out:
 *    n/a
 *
 * Description:
 *      Send Hello
 * Assumptions:
 *
 *****************************************************************************/
void USB_SendTuttiInterventi(void)
{
  word cntInt;
  byte _myarr[MAX_LEN_PAYLOAD];
  byte *ritFun;

    
  cntInt = 0;
  do {
    ritFun = ReadIntervento(cntInt++);
    _memcpy(_myarr,ritFun, INTERVENTO_LENGTH);          
    USB_PktSend(_myarr,INTERVENTO_LENGTH); 
  } while ((ritFun[0] != 0xFF) && (cntInt < NUM_MAX_INTERV));
  
  // Se uscito per raggiunto NUM_MAX_INTERV -> invio messaggio con tutti 0xFF
  if (cntInt >= NUM_MAX_INTERV) {    
    for (cntInt=0;cntInt<INTERVENTO_LENGTH;cntInt++) _myarr[cntInt] = 0xFF;  
    USB_PktSend(_myarr,INTERVENTO_LENGTH);   
  }
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
      //ResetPkt_ndx();
      
    }
    
    
    
    
    if (USBPktReady) {
      
      // Analizzare messaggio ricevuto
      
      if (usbPayloadLen==0) {      
        //USB_PktSend(USBtoSend,0); 
      } else {
        switch (usbrxar[usbNumByteLen+1]) {

          case REQ_HELLO:
          USB_SendHello();
          break;


          case REQ_LIST_INTERV:          
            USB_SendTuttiInterventi();      
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
