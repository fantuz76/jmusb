/****************************************************************************
 *
 *            Copyright (c) 2006-2007 by CMX Systems, Inc.
 *
 * This software is copyrighted by and is the sole property of
 * CMX.  All rights, title, ownership, or other interests
 * in the software remain the property of CMX.  This
 * software may only be used in accordance with the corresponding
 * license agreement.  Any unauthorized use, duplication, transmission,
 * distribution, or disclosure of this software is expressly forbidden.
 *
 * This Copyright notice may not be removed or modified without prior
 * written consent of CMX.
 *
 * CMX reserves the right to modify this software without notice.
 *
 * CMX Systems, Inc.
 * 12276 San Jose Blvd. #511
 * Jacksonville, FL 32223
 * USA
 *
 * Tel:  (904) 880-1840
 * Fax:  (904) 880-1632
 * http: www.cmx.com
 * email: cmx@cmx.com
 *
 ***************************************************************************/
 
#include "usb-drv/usb.h"
#include "target.h"
#include "usb_cdc.h"
#include "uart-drv/uart.h"

/****************************************************************************
 ************************** Macro definitions *******************************
 ***************************************************************************/
/* none */

/****************************************************************************
 ************************** Function predefinitions. ************************
 ***************************************************************************/
/* none */

/****************************************************************************
 ************************** Global variables ********************************
 ***************************************************************************/
/* none */

/****************************************************************************
 ************************** Module variables ********************************
 ***************************************************************************/
/* none */

/****************************************************************************
 ************************** Function definitions ****************************
 ***************************************************************************/
int main()
{
  int cdc_in;
  int uart_in; 

  hw_init();

  cdc_in=uart_in=0xff+1;

  usb_cfg_init();

  cdc_init();
  
  /* 9600 bps, 1 sto bit, no parity, 8 data bits */
  uart_init(9600, 1, 'n', 8);

  /* This loop will gateway charactert from the UART to the USB and back. */
  while(1)
  {
    cdc_process();
    /* Set uart line coding to match USB CDC settings if neded. */
    if (cdc_line_coding_changed())
    {
       line_coding_t l;
       hcc_u8 parity[]="noe";
       cdc_get_line_coding(&l);
       uart_init(l.bps, l.nstp+1, parity[l.parity], l.ndata);
    }
    
    /* if cdc2uart buffer is not empry */
    if (cdc_in <= 0xff)
    {
      /* Try to send */
      int uart_out=uart_putch((hcc_u8)cdc_in);
      /* if successfull, set cdc2uart buffer to empty */
      if (uart_out==cdc_in)
      {
        cdc_in=0xff+1;
      }
    }
    
    /* if cdc2uart buffer is not busy and cdchas received somethign,
       fill cdc2uart buffer  */
    if (cdc_in > 0xff && cdc_kbhit())
    {
      cdc_in=cdc_getch();        
    }

    /* if uart2cdc buffer is empty */
    if (uart_in <= 0xff)
    {
      /* try to send on CDC */
      int cdc_out=cdc_putch((hcc_u8)uart_in);
      /* if success, set uart2cdc buffer empty */
      if (cdc_out==uart_in)
      {
        uart_in=0xff+1;
      }
    }

    /* if uart2cdc buffer is empty and uart has received something,
       fill uart2cdc buffer */
    if (uart_in > 0xff && uart_kbhit())
    {
      uart_in=uart_getch();
    }
  }
  return 0;
}
/****************************** END OF FILE **********************************/
