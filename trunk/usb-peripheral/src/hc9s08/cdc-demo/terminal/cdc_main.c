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
#include "terminal/hcc_terminal.h"
#include "utils/utils.h"
#include "protocol.h"


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
static void cmd_led(char *param)
{
  static led_on=0;
  param++;  
  if (led_on)
  {
    led_on=0;
    PTED_PTED2=0;
  }
  else
  {
    led_on=1;
    PTED_PTED2=1;
  }

}

static const command_t led_cmd = {
  "led", cmd_led, "Toggles leds state."
};

int main()
{
  hw_init();
  
  usb_cfg_init();

  cdc_init();
  //terminal_init(cdc_putch, cdc_getch, cdc_kbhit);
  //(void)terminal_add_cmd((command_t*)&led_cmd);
  
  comm_init(cdc_putch, cdc_getch, cdc_kbhit);

  /* This loop will gateway charactert from the UART to the USB and back. */
  while(1)
  {
    comm_process();
    //terminal_process();
    cdc_process();
  }
  return(0);
}
/****************************** END OF FILE **********************************/
