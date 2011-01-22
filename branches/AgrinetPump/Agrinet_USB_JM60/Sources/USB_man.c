#include "USB_man.h"
#include "usb_cdc.h" 
#include "Agrinet_pump.h"

#include <stdtypes.h>





/*****************************************************************************
 * Module variables.
 *****************************************************************************/

static byte usbNumByteLen;
static byte usbPayloadLen;

//static char usbrxar[MAX_LEN_CMD];

static byte usbrxar_ndx;
//static word TimerUSB_1;




static byte_def _USB_flags1;
#define USB_flags1              _USB_flags1.BYTE
#define USBPktReady             _USB_flags1.BIT.B0
#define USBSendInterventi_en    _USB_flags1.BIT.B1
#define USBSendHello_en         _USB_flags1.BIT.B2



byte state_USBSend;




/*****************************************************************************
 * Funzioni
 *****************************************************************************/


/*****************************************************************************
* Name:
*    ResetPkt_ndx
*
* Description: Reset pkt in ricezione da USB
*****************************************************************************/
void ResetPkt_ndx(void){
  usbrxar_ndx = 0;
  usbPayloadLen = 0;
  usbNumByteLen = 0;        
  USBPktReady = FALSE;
}




/*****************************************************************************
* Name:
*    comm_init
* Description:
*
*****************************************************************************/
void USB_comm_init(void)
{
  usb_cfg_init();         // Init Registri USB micro
  Init_USB_man();         // Init Variabili USB per comunicazione
}                                


void Init_USB_man(void){
  cdc_init();   
  //usbrxar[sizeof(usbrxar)-1]='\0';   
  
  ResetPkt_ndx();
  USB_flags1=0x00;    
}


/*****************************************************************************
 * Name:
 *    USB_PktSend
 * Description:
 *    _toSend è l'array del payload da inviare
 *    _len    è la lunghezza dell'array
 *    _firstPayload   Se <> 0, è il byte da aggiungere all'inizio del payload 
 *                    che risulterà così incrementato di 1. Serve per ripetere il comando.
 *
 *    Send Pkt (vedere documento)
 *    FIRST_BYTE + (LENGTH) + PAYLOAD + CHKSUM
 *****************************************************************************/
void USB_PktSend(byte *_toSend, byte _len, byte _firstPayload)
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
    byteToSend = (byte)(_firstPayload==0 ? _len : _len+1);
    while(byteToSend != (char)cdc_putch(byteToSend))
      ;
    cks += byteToSend;
  }

  // _firstPayload (primo byte del payload, serve se devo ripetere comando ad es REQ_HELLO)
  if (_firstPayload != 0) {
    byteToSend = _firstPayload;
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
 * Description:
 *      Send Hello
 *****************************************************************************/
void USB_SendHello(void)
{ 
  switch (state_USBSend) {
    case 0:        
      allarme_in_lettura = 2000;
      state_USBSend = 1;
    break;
    
    case 1:
      if (allarme_in_lettura == 2001) {        
        USB_PktSend(buffer_USB,96, REQ_HELLO);          
        USBSendHello_en = FALSE; 
      }
    break;
    
  }       
}
    
 


/*****************************************************************************
 * Name:
 *    USB_SendTuttiInterventi
 * Description:
 *      Send Interventi.
 *****************************************************************************/
void USB_SendTuttiInterventi(void)
{  
  word cntInt;
  byte _myarr[INTERVENTO_LENGTH];
  switch (state_USBSend) {
    case 0:        
      allarme_in_lettura = 1;
      state_USBSend = 1;
      pronto_alla_risposta = 0;
    break;
    
    case 1:
      if (pronto_alla_risposta) {    
        USB_PktSend(buffer_USB,INTERVENTO_LENGTH,0);          
        allarme_in_lettura++;
        pronto_alla_risposta = 0; 
        
        if ((buffer_USB[0] == 0xFF) || (allarme_in_lettura >= totale_indicazioni_fault))
          state_USBSend = 2;        
      }
    break;
    
    case 2:
      for (cntInt=0;cntInt<INTERVENTO_LENGTH;cntInt++) _myarr[cntInt] = 0xFF;    
      USB_PktSend(_myarr,INTERVENTO_LENGTH,0);   
      USBSendInterventi_en = FALSE;
      state_USBSend= 0;
    break;
    
  }

 
}  
  
  
  
/*****************************************************************************
 * Name:
 *    USB_ReadRequest
 * Description:
 *      Gestisce e impacchetta richieste provenienti da PC
 *      Se ricevuto un pkt corretto Setta USBPktReady = TRUE
 *****************************************************************************/
void USB_ReadRequest(void)
{  
  byte i;
  byte cks;    

  // finchè c'è qualcosa nel buffer USB
  // Se ci sono più di 32 byte entro + volte qua  
  while((cdc_kbhit)())
  {      
  
    char c;
    c=(char)(cdc_getch)();        
    buffer_USB[usbrxar_ndx++]=c;
    
    
    if (usbrxar_ndx >= 96)   // Overflow 
      ResetPkt_ndx();
      
    
    if (usbrxar_ndx-1==0) {              
   
      // --> è primo byte rx
      if ((buffer_USB[usbrxar_ndx-1] & 0xF0) == FIRST_BYTE_EMPTY) {        
        // C'è la Lunghezza?  
        usbNumByteLen = (buffer_USB[usbrxar_ndx-1] & 0x01);
      } else {        
        ResetPkt_ndx();         // Error - pkt unknown
      }      

    } else if ( (usbNumByteLen==1) &&  (usbrxar_ndx-1==1)) {
      // --> Sono in lunghezza      
        usbPayloadLen = buffer_USB[1];         
    
    } else if ((usbrxar_ndx-1 >= usbNumByteLen+1)  && (usbrxar_ndx-1 < usbPayloadLen+usbNumByteLen+1)) {
      // --> Sono in PayLoad
            
      
    } else if (usbrxar_ndx-1 == usbPayloadLen+usbNumByteLen+1) {
      // --> Sono in ChkSum
      for (i=0,cks=0; i<usbrxar_ndx-1; cks += buffer_USB[i++]);
              
      if (cks != buffer_USB[i])
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
 
 
}  



/*****************************************************************************
 * Name:
 *    USB_HandleRequest
 * Description:
 *      Se è stato ricevuto un pkt corretto da USB_ReadRequest
 *      Qua dentro viene preparata l'azione corrispondente
 *****************************************************************************/
 void USB_HandleRequest(void){
  if (USBPktReady) {  
    // Analizzare messaggio ricevuto    
    
    if (usbPayloadLen==0) {
      USB_PktSend(0,0,0);     // Messaggio vuoto       
    } else {
      switch (buffer_USB[usbNumByteLen+1]) {

        case REQ_HELLO:
          USBSendHello_en = TRUE;                    
          state_USBSend = 0;
        break;


        case REQ_LIST_INTERV:                    
          USBSendInterventi_en =TRUE;
          state_USBSend = 0;
        break;

        case REQ_ERR_0:
          USB_PktSend(0,0,0);     // Messaggio vuoto
        break;

        default:
          USB_PktSend(0,0,0);     // Messaggio vuoto
        break;          
      }
    
    }
    
    ResetPkt_ndx();             // Resetta anche USBPktReady    
  }
}


/*****************************************************************************
* Name:
*    comm_proces
*
* Description: Gestione comunicazione USB
*                
*****************************************************************************/
void USB_comm_process(void)
{
  // Se è attiva una richiesta di invio la esegue
  if (USBSendHello_en){      
    USB_SendHello();    
  } else if (USBSendInterventi_en) {
    USB_SendTuttiInterventi();      
  }
  
  USB_ReadRequest();
 
  USB_HandleRequest();
}




/*****************************************************************************
 * Name:
 *    USB_time_sw
 *
 * Description:  Richiamata da interrupt Main ogni 20ms 
 *****************************************************************************/  
/*void USB_time_sw(void){  
  if (TimerUSB_1 != 0xFFFF) 
  TimerUSB_1--;  
}

void StartTimer(word *_timerToStart, word _valToStart){
  *_timerToStart = _valToStart;
}

byte isTimerStopped(word _timerToCheck){
  return (_timerToCheck == 0xFFFF);
}
*/

/*****************************************************************************
 * Name:
 *    print
 *
 * Description:
 *    Print the specified string.
 *****************************************************************************/
/*void print(char *s)
{
  while(*s)
  {
    while(*s != (char)cdc_putch(*s))
      ;
    s++;
  }
} */