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
#ifndef _HID_USB_CONFIG_H_
#define _HID_USB_CONFIG_H_
#include "hcc_types.h"
#include "usb-drv/usb.h"

#define EP0_PACKET_SIZE 8u
#define HID_IT_EP_NDX   1u
/***************************************************************************
 * Functions exported to the main application.
 **************************************************************************/
 typedef enum {
    dm_kbd,
    dm_mouse,
    dm_generic
} device_mode_t;

extern void set_mode(device_mode_t mode);
extern void usb_cfg_init(void);
#endif
/****************************** END OF FILE **********************************/
