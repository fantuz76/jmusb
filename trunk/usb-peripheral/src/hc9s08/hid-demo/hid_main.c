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
#include "hid_mouse.h"
#include "hid_kbd.h"
#include "hid_generic.h"
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
  hcc_u8 tmp;
  hw_init();

  /* See in what mode should we run. */
  tmp = (hcc_u8)(SW1_ACTIVE() ? 1 : 0);
  tmp |= (hcc_u8)(SW2_ACTIVE() ? 2 : 0);

  /* Start the right HID application. */
  switch(tmp)
  {
  default:
  case 0:
    usb_cfg_init();
    set_mode(dm_mouse);
    (void)hid_mouse();
    break;
  case 1:
    usb_cfg_init();
    set_mode(dm_kbd);
    hid_kbd();
    break;
  case 2:
    usb_cfg_init();
    set_mode(dm_generic);
    hid_generic();
    break;
  }
  /* We will never get here. */
  return 0;
}
/****************************** END OF FILE **********************************/
