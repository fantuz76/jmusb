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
#include "hid_generic.h"
#include "hid.h"
#include "hcc_types.h"
#include "target.h"

/*****************************************************************************
 * Generic HID device main loop. 
 ****************************************************************************/

#ifdef ON_THE_GO
extern void busy_wait(void);
extern hcc_u8 device_stp;
#else
#define busy_wait()
static hcc_u8 device_stp=0;
#endif


void hid_generic(void)
{
  hcc_u8 out_report;
  hcc_u8 in_report;
  hcc_u8 old_sw;

  HID_init(500, 0);

  out_report=hid_add_report(rpt_out, 0, 1);
  in_report=hid_add_report(rpt_in, 0, 1);
  old_sw=0xff;

  while(!device_stp)
  {
    hid_process();

    /* Send switch status. */
    if (!hid_report_pending(in_report))
    {
      hcc_u8 tmp=0;
      
      if (SW1_ACTIVE())
      {
        tmp |= 1;
      }
      if (SW2_ACTIVE())
      {
        tmp |= 2;
      }
      if (old_sw != tmp)
      {
        old_sw=tmp;
        hid_write_report(in_report, &tmp);
      }
    }

    /* Set status leds if needed. */    
    if (hid_report_pending(out_report))
    {
      hcc_u8 leds;
      hid_read_report(out_report, &leds);
      
      if (leds & 1)
      {
        LED1_ON;
      }
      else
      {
        LED1_OFF;
      }
      
      if (leds & 2)
      {
        LED2_ON;
      }
      else
      {
        LED2_OFF;
      }

      if (leds & 4)
      {
        LED3_ON;
      }
      else
      {
        LED3_OFF;
      }
      
      if (leds & 8)
      {
        LED4_ON;
      }
      else
      {
        LED4_OFF;
      }      
    }
    
    busy_wait();    
  }
}
/****************************** END OF FILE **********************************/
