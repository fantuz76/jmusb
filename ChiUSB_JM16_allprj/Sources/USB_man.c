#include "usb_cdc.h" 
#include "USB_man.h"
#include <stdtypes.h>
#include <utils.h>

#include <stdlib.h>
#include <string.h>



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
*
* Assumptions:
*    --
*****************************************************************************/
void comm_init(void)
{

  usbrxar[sizeof(usbrxar)-1]='\0';
   
  ResetPkt_ndx();

  srand(122);
  
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
  
  
  // Invia matricola 
  _myarr[1] = 'a';//0x23;
  _myarr[2] = 'b';//0x45;
  _myarr[3] = 0x67;    
  _myarr[4] = 0x55;
  
  // Ore lavoro 
  _myarr[5] = 0x00;
  _myarr[6] = 0x03;
  _myarr[7] = 0x55;
  _myarr[8] = 0x44;
  
  
  // Fw Ver
  _myarr[9] = 0x01;
  _myarr[10] = 0x02;

  
  // Hw Ver
  _myarr[11] = 0x03;
  _myarr[12] = 0x04;
  
  USB_PktSend(_myarr,13);   

}
    
 



byte *ReadIntervento(word _PosMem) {

  
  // Tipo intervento casuale
  arrIntervento[0] = 1+(rand() % 28);
  
  
  if (arrIntervento[0] == 0 )  arrIntervento[0] = 1;  
  if (arrIntervento[0] == 19 )  {
    arrIntervento[0] = 20;
  }
  
  if ((arrIntervento[0] > 2) && (arrIntervento[0]<10))  {
    arrIntervento[0] = 1+(rand() % 28);
    if ((arrIntervento[0] > 2) && (arrIntervento[0]<10))  {

      arrIntervento[0] = 1+(rand() % 28);
      if ((arrIntervento[0] > 2) && (arrIntervento[0]<10))  {
        arrIntervento[0] = 18;  
      }    
    }
  } 

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
    memcpy(_myarr,ritFun, INTERVENTO_LENGTH);          
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
* Description: Gestione comunicazione USB
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
  
  
  // finch� c'� qualcosa nel buffer USB
  // Se ci sono pi� di 32 byte entro + volte qua  
  while((cdc_kbhit)())
  {      
    char c;
    c=(char)(cdc_getch)();        
    usbrxar[usbrxar_ndx++]=c;
    
    if (usbrxar_ndx >= MAX_LEN_CMD)   // Overflow 
      ResetPkt_ndx();
    
   
    
    if (usbrxar_ndx-1==0) {    
      // --> � primo byte rx
      if ((usbrxar[usbrxar_ndx-1] & 0xF0) == FIRST_BYTE_EMPTY) {        
        // C'� la Lunghezza?  
        usbNumByteLen = (usbrxar[usbrxar_ndx-1] & 0x01);
      } else {        
        ResetPkt_ndx();         // Error - pkt unknown
      }      

    } else if ( (usbNumByteLen==1) &&  (usbrxar_ndx-1==1)) {
      // --> Sono in lunghezza      
        usbPayloadLen = usbrxar[1];         
    
    } else if ((usbrxar_ndx-1 >= usbNumByteLen+1)  && (usbrxar_ndx-1 < usbPayloadLen+usbNumByteLen+1)) {
      // --> Sono in PayLoad
            
      
    } else if (usbrxar_ndx-1 == usbPayloadLen+usbNumByteLen+1) {
      // --> Sono in ChkSum
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
    
  }
}
