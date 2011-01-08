#include "USB_man.h"
#include "usb_cdc.h" 
#include "Agrinet_pump.h"

#include <stdtypes.h>
#include <stdlib.h>
#include <string.h>




/*****************************************************************************
 * Module variables.
 *****************************************************************************/

static byte usbNumByteLen;
static byte usbPayloadLen;

static char usbrxar[MAX_LEN_CMD];

static byte usbrxar_ndx;
static word TimerUSB_1;




static byte_def _USB_flags1;
#define USB_flags1              _USB_flags1.BYTE
#define USBPktReady             _USB_flags1.BIT.B0
#define USBSendInterventi_en    _USB_flags1.BIT.B1
#define USBSendHello_en         _USB_flags1.BIT.B2



byte state_SendTuttiInterventi;

void ResetPkt_ndx(void){
  usbrxar_ndx = 0;
  usbPayloadLen = 0;
  usbNumByteLen = 0;        
  USBPktReady = FALSE;
}


void StartTimer(word *_timerToStart, word _valToStart){
  *_timerToStart = _valToStart;
}

byte isTimerStopped(word _timerToCheck){
  return (_timerToCheck == 0xFFFF);
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
void USB_comm_init(void)
{
  usb_cfg_init();
  
  Init_USB_man();
}                                


void Init_USB_man(void){
  cdc_init();
   
  usbrxar[sizeof(usbrxar)-1]='\0';
   
  ResetPkt_ndx();

  srand(122);
  
  USB_flags1=0x00;    
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

  if (USBSendHello_en){      
    _myarr[0] = REQ_HELLO;  // Indica risposta ad Hello
        
    // Invia matricola 
    _myarr[1] = 'a';
    _myarr[2] = 'b';
    _myarr[3] = 'c';    
    _myarr[4] = 'd';
    
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
    
    USBSendHello_en = FALSE;
  }

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

 
  if (USBSendInterventi_en) {
  
    switch (state_SendTuttiInterventi) {
      case 0:        
        allarme_in_lettura = 1;
        state_SendTuttiInterventi = 1;
        pronto_alla_risposta = 0;
      break;
      
      case 1:
        if (pronto_alla_risposta) {
          memcpy(_myarr,buffer_USB, INTERVENTO_LENGTH);      
          USB_PktSend(_myarr,INTERVENTO_LENGTH);          
          allarme_in_lettura++;
          pronto_alla_risposta = 0; 
          
          if ((buffer_USB[15] == 0xFF) || (allarme_in_lettura >= totale_indicazioni_fault))
            state_SendTuttiInterventi = 2;
          
        }
      break;
      
      case 2:
        for (cntInt=0;cntInt<INTERVENTO_LENGTH;cntInt++) _myarr[cntInt] = 0xFF;    
        USB_PktSend(_myarr,INTERVENTO_LENGTH);   
        USBSendInterventi_en = FALSE;
        state_SendTuttiInterventi= 0;
      break;
      
    }

  }

}  
  
  
/*****************************************************************************
 * Name:
 *    USB_time_sw
 * In:
 *   
 * Out:
 *    n/a
 *
 * Description:  Richiamata da interrupt Main ogni 20ms
 *  
 * Assumptions:
 *
 *****************************************************************************/  
void USB_time_sw(void){
  
  if (TimerUSB_1 != 0xFFFF) 
  TimerUSB_1--;
  
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
void USB_comm_process(void)
{

  byte USBtoSend[MAX_LEN_PAYLOAD];
  byte i;
  byte cks;
  

  USB_SendHello();    
  USB_SendTuttiInterventi();      
  
  
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
   
      // --> è primo byte rx
      if ((usbrxar[usbrxar_ndx-1] & 0xF0) == FIRST_BYTE_EMPTY) {        
        // C'è la Lunghezza?  
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
    
    
    
  }
  
  
  if (USBPktReady) {
  

    
    // Analizzare messaggio ricevuto
    
    if (usbPayloadLen==0) {      
      //USB_PktSend(USBtoSend,0); 
    } else {
      switch (usbrxar[usbNumByteLen+1]) {

        case REQ_HELLO:
          USBSendHello_en = TRUE;          
        break;


        case REQ_LIST_INTERV:                    
          USBSendInterventi_en =TRUE;
          state_SendTuttiInterventi= 0;
        break;

        case REQ_ERR_0:          
        break;

        default:
          USBtoSend[0] = 0x55;
          USB_PktSend(USBtoSend,1);
        break;          
      }
    
    }
    
    ResetPkt_ndx();             // Resetta anche USBPktReady
    
  }



  
}
