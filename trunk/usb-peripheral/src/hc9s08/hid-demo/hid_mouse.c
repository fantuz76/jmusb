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
#include "hid.h"
#include "hid_mouse.h"

/****************************************************************************
 ************************** Macro definitions *******************************
 ***************************************************************************/
/* Class specific requests. */
#define HIDRQ_GET_REPORT    0x1
#define HIDRQ_GET_IDLE      0x2
#define HIDRQ_GET_PROTOCOL  0x3

#define HIDRQ_SET_REPORT    0x9
#define HIDRQ_SET_IDLE      0xa
#define HIDRQ_SET_PROTOCOL  0xb

/* Descriptor type values for HID descriptors. */
#define GHIDD_HID_DESCRIPTOR      0x21
#define GHIDD_REPORT_DESCRIPTOR   0x22
#define GHIDD_PHYSICAL_DESCRIPTOR 0x23

/* Accessing report items. */
#define DIR_REP_BUTTONS(h)  ((h)[0])
#define DIR_REP_X(h)	      ((h)[1])
#define DIR_REP_Y(h)        ((h)[2])

/****************************************************************************
 ************************** Type definitions ********************************
 ***************************************************************************/
typedef signed char hid_report_t[3];

/****************************************************************************
 ************************** Function predefinitions. ************************
 ***************************************************************************/
void mou_got_reset(void);

#ifdef ON_THE_GO
extern void busy_wait(void);
extern hcc_u8 device_stp;
#else
#define busy_wait()
static hcc_u8 device_stp=0;
#endif

/****************************************************************************
 ************************** Global variables ********************************
 ***************************************************************************/
/* none */

/****************************************************************************
 ************************** Module variables ********************************
 ***************************************************************************/
static hid_report_t hid_report;

/****************************************************************************
 ************************** Function definitions ****************************
 ***************************************************************************/


/*****************************************************************************
 * USB callback function. Is called by the USB driver if an USB reset event
 * occuers.
 ****************************************************************************/
void mou_got_reset(void)
{
  /* do some initialisation. */
  DIR_REP_BUTTONS(hid_report)=0;
  DIR_REP_X(hid_report)=0;
  DIR_REP_Y(hid_report)=0;
}

/*****************************************************************************
 * This function will move the mouse pointer from left to right and back in
 * an endless loop.
 ****************************************************************************/
int hid_mouse(void)
{
  int x=0;
  const int delta=5;
  hcc_u8 in_report;
  
  HID_init(0, 0);
  
  in_report=hid_add_report(rpt_in, 0, 3);
  
  while(!device_stp)
  {
    hid_process();
    if (!hid_report_pending(in_report))
    {
      if (DIR_REP_X(hid_report) >= 0)
      {
        x+=delta;
        if (x >= 30 )
       {
         DIR_REP_X(hid_report) = (hcc_u8)(-1*delta);
       }
      }
      else
      {
        x-=delta;
        if (x <= 0)
        {
          DIR_REP_X(hid_report) = delta;
        }
      }    
      hid_write_report(in_report, (hcc_u8*)hid_report);
    }  
    
    busy_wait();
  }
  return(0);
}
/****************************** END OF FILE **********************************/
