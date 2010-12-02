#include "usb_cdc.h" 
#include "terminal/hcc_terminal.h"
#include "protocol.h"


/*****************************************************************************
 * Module variables.
 *****************************************************************************/

static hcc_u8 usbNumPktLen;

static char usbrxar[MAX_LEN_CMD];
static hcc_u8 usbrxar_ndx;

//static hcc_u8 n_cmd;
//static command_t * cmds[MAX_CMDS];


static int (*putch)(char);
static int (*getch)(void);
static int (*kbhit)(void);



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
  usbrxar_ndx=0;

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
hcc_u8 ParseFirstByte(hcc_u8 _byteToParse) 
{
  if ((_byteToParse & 0xF0) == 0x80) {    
    usbNumPktLen = (_byteToParse & 0x03);
  } else {
    // Error - pkt unknown
    return(0);        
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

  // finchè c'è qualcosa nel buffer USB
  // Se ci sono più di 32 byte entro + volte qua
  
  while((cdc_kbhit)())
  {
      
    char c;
    c=(char)(cdc_getch)();        
    usbrxar[usbrxar_ndx++]=c;
    
    __asm("nop");
    
    if (usbrxar_ndx==1) {
      // primo byte rx
 /*     if (ParseFirstByte(usbrxar[usbrxar_ndx-1]) == 0) {
        // Parse KO        
        usbrxar_ndx = 0;
      }
      
 */     
    } else if (usbNumPktLen) {
      
    }
    
    
    if (usbrxar_ndx==MAX_LEN_CMD) {
      usbrxar_ndx=0;
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
