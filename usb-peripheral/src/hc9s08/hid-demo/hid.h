/***************************************************************************
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
#ifndef _HID_H_
#define _HID_H_

#include "hcc_types.h"
#include "usb-drv/usb.h"

typedef struct {
  void *start_addr;
  hcc_u16 size;
} descriptor_info_t;

typedef enum {
  rpt_in,
  rpt_out,
  rpt_feature
} hid_report_type;


  
extern void HID_init(hcc_u16 default_idle_time, hcc_u8 ifc_number);
void hid_process(void);
extern hcc_u8 hid_add_report(hid_report_type type, hcc_u8 id, hcc_u8 size);
extern void hid_write_report(hcc_u8 r, hcc_u8 *data);
extern void hid_read_report(hcc_u8 r, hcc_u8 *data);
extern hcc_u8 hid_report_pending(hcc_u8 r);
extern hcc_u16 hid_get_idle_time(void);
extern callback_state_t usb_ep0_hid_callback(void);

extern descriptor_info_t *get_hid_descriptor(void);
extern descriptor_info_t *get_report_descriptor(void);
extern descriptor_info_t *get_physical_descriptor(hcc_u8 id);
extern void got_usb_reset(void);


#endif
