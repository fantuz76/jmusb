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
#include "hid.h"

#ifdef ON_THE_GO
#include "otg-drv/otg.h"

#else
#define USB_FILL_OTG_DESC(hnp, srp) \
  (hcc_u8)3, 0x9, 0

#endif

#define USB_FILL_HID_DESC(length, hid_rel, country, ndesc, desc_type, desc_len) \
  (hcc_u8)(length), 0x21, (hcc_u8)(hid_rel), (hcc_u8)((hid_rel)>>8), (hcc_u8)(country)\
  ,(hcc_u8)(ndesc), (hcc_u8)(desc_type), (hcc_u8)(desc_len), (hcc_u8)((desc_len)>>8)


hcc_u8 string_descriptor0[4] = {4, 0x3, 0x09, 0x04 };
hcc_u8 str_manufacturer[26] = { 26, 0x3, 'C',0, 'M',0, 'X',0, ' ',0
                  , 'S',0, 'y',0, 's',0, 't',0, 'e',0, 'm',0, 's',0, ' ',0};

hcc_u8 buf_ep00[EP0_PACKET_SIZE];
hcc_u8 buf_ep01[EP0_PACKET_SIZE];

/************************************ KEYBOARD ***************************/
#define KBD_IFC_INDEX   0u
#define KBD_VENDOR_ID   0xc1cau
#define KBD_PRODUCT_ID  (0x1u)
#define KBD_DEVICE_REL_NUM  0x0

const hcc_u8 kbd_config[44] = { 44, 0x3, 'D',0, 'e',0, 'f',0, 'a',0, 'u',0
                  , 'l',0, 't',0, ' ',0, 'c',0, 'o',0, 'n',0, 'f',0, 'i',0
                  , 'g',0, 'u',0, 'r',0, 'a',0, 't',0, 'i',0, 'o',0, 'n',0};
const hcc_u8 kbd_interface[26] = { 26, 0x3, 'H',0, 'I',0, 'D',0, '-',0, 'K',0
                  , 'e',0, 'y',0, 'b',0, 'o',0, 'a',0, 'r',0, 'd',0 };
const hcc_u8 kbd_serail_number[10] = { 10, 0x3, 'V',0, '0',0, '.',0, '1',0};
const hcc_u8 kbd_product[86] = { 86, 0x3, 'U',0, 'S',0, 'B',0, ' ',0, 'H',0
                  , 'I',0, 'D',0, ' ',0, 'k',0, 'e',0, 'y',0, 'b',0, 'o',0
                  , 'a',0, 'r',0, 'd',0, ' ',0, 'd',0, 'e',0, 'm',0, 'o',0
                  , ' ',0, 'f',0, 'o',0, 'r',0, ' ',0, 'H',0, 'C',0, '9',0
                  , 'S',0, '0',0, '8',0, 'M',0, 'J',0, ' ',0, 'd',0, 'e',0
                  , 'v',0, 'i',0, 'c',0, 'e',0, 's',0};

const hcc_u8 * kbd_string_descriptors[] = {
  string_descriptor0, str_manufacturer, kbd_product, kbd_serail_number
  , kbd_config, kbd_interface
};

const hcc_u8 kbd_device_descriptor[] = {
  USB_FILL_DEV_DESC(0x0101, 0, 0, 0, EP0_PACKET_SIZE, KBD_VENDOR_ID, KBD_PRODUCT_ID
      , KBD_DEVICE_REL_NUM, 1, 2, 3, 1)
};
const hcc_u8 kbd_report_descriptor[63] = {
    0x05, 0x01,                    /* USAGE_PAGE (Generic Desktop) */
    0x09, 0x06,                    /* USAGE (Keyboard) */
    0xa1, 0x01,                    /* COLLECTION (Application) */
    0x05, 0x07,                    /*   USAGE_PAGE (Keyboard) */
    0x19, 0xe0,                    /*   USAGE_MINIMUM (Keyboard LeftControl) */
    0x29, 0xe7,                    /*   USAGE_MAXIMUM (Keyboard Right GUI) */
    0x15, 0x00,                    /*   LOGICAL_MINIMUM (0) */
    0x25, 0x01,                    /*   LOGICAL_MAXIMUM (1) */
    0x75, 0x01,                    /*   REPORT_SIZE (1) */
    0x95, 0x08,                    /*   REPORT_COUNT (8) */
    0x81, 0x02,                    /*   INPUT (Data,Var,Abs) modifier keys (CTRL, ALT, etc...*/
    0x95, 0x01,                    /*   REPORT_COUNT (1) */
    0x75, 0x08,                    /*   REPORT_SIZE (8) */
    0x81, 0x03,                    /*   INPUT (Cnst,Var,Abs) filupp to byte boundary */
    0x95, 0x05,                    /*   REPORT_COUNT (5) */
    0x75, 0x01,                    /*   REPORT_SIZE (1) */
    0x05, 0x08,                    /*   USAGE_PAGE (LEDs) */
    0x19, 0x01,                    /*   USAGE_MINIMUM (Num Lock) */
    0x29, 0x05,                    /*   USAGE_MAXIMUM (Kana) */
    0x91, 0x02,                    /*   OUTPUT (Data,Var,Abs) LED state pc->kbd */
    0x95, 0x01,                    /*   REPORT_COUNT (1) */
    0x75, 0x03,                    /*   REPORT_SIZE (3 */
    0x91, 0x03,                    /*   OUTPUT (Cnst,Var,Abs) filupp to byte boundary */
    0x95, 0x06,                    /*   REPORT_COUNT (6) */
    0x75, 0x08,                    /*   REPORT_SIZE (8) */
    0x15, 0x00,                    /*   LOGICAL_MINIMUM (0) */
    0x25, 0x65,                    /*   LOGICAL_MAXIMUM (101) */
    0x05, 0x07,                    /*   USAGE_PAGE (Keyboard) */
    0x19, 0x00,                    /*   USAGE_MINIMUM (Reserved (no event indicated)) */
    0x29, 0x65,                    /*   USAGE_MAXIMUM (Keyboard Application) */
    0x81, 0x00,                    /*   INPUT (Data,Ary,Abs) array for pressed keys */
    0xc0                           /* END_COLLECTION */
};

const hcc_u8 kbd_config_descriptor[] = {
  USB_FILL_CFG_DESC(9+3+9+9+7, 1, 1, 4, CFGD_ATTR_SELF_PWR, 0),
  USB_FILL_OTG_DESC(1, 1),
  USB_FILL_IFC_DESC(KBD_IFC_INDEX, 0, 1, 0x3, 0x1, 0x1, 5), /* HID, boot, keyboard (3/1/1) */
  USB_FILL_HID_DESC(9, 0x0100, 0x0, 1, 0x22, sizeof(kbd_report_descriptor)),
  USB_FILL_EP_DESC(0x1, 1, 0x3, 8, 0x20),
};

/*************************************************************************/
/************************************* MOUSE *****************************/
#define MOU_IFC_INDEX   0u
#define MOU_VENDOR_ID   0xc1cau
#define MOU_PRODUCT_ID  (2u)
#define MOU_DEVICE_REL_NUM  0x0

const hcc_u8 mou_config[44] = { 44, 0x3, 'D',0, 'e',0, 'f',0, 'a',0, 'u',0
                  , 'l',0, 't',0, ' ',0, 'c',0, 'o',0, 'n',0, 'f',0, 'i',0
                  , 'g',0, 'u',0, 'r',0, 'a',0, 't',0, 'i',0, 'o',0, 'n',0};
const hcc_u8 mou_interface[20] = { 20, 0x3, 'H',0, 'I',0, 'D',0, '-',0, 'M',0
                  , 'o',0, 'u',0, 's',0, 'e',0, };
const hcc_u8 mou_serail_number[10] = { 10, 0x3, 'V',0, '0',0, '.',0, '1',0};
const hcc_u8 mou_product[80] = { 80, 0x3, 'U',0, 'S',0, 'B',0, ' ',0, 'H',0
                  , 'I',0, 'D',0, ' ',0, 'M',0, 'o',0, 'u',0, 's',0
                  , 'e',0, ' ',0, 'd',0, 'e',0, 'm',0, 'o',0, ' ',0, 'f',0
                  , 'o',0, 'r',0, ' ',0, 'H',0, 'C',0, '9',0, 'S',0, '0',0
                  , '0',0, 'J',0, 'M',0, ' ',0, 'd',0, 'e',0, 'v',0, 'i',0
                  , 'c',0, 'e',0, 's',0};

const hcc_u8 * mou_string_descriptors[] = {
  string_descriptor0, str_manufacturer, mou_product, mou_serail_number
  , mou_config, mou_interface
};

const hcc_u8 mou_device_descriptor[] = {
  USB_FILL_DEV_DESC(0x0101, 0, 0, 0, EP0_PACKET_SIZE, MOU_VENDOR_ID, MOU_PRODUCT_ID
      , MOU_DEVICE_REL_NUM, 1, 2, 3, 1)
};

const hcc_u8 mou_report_descriptor[50] = {
    0x05, 0x01,                    /* USAGE_PAGE (Generic Desktop) */
    0x09, 0x02,                    /* USAGE (Mouse) */
    0xa1, 0x01,                    /* COLLECTION (Application) */
    0x09, 0x01,                    /*   USAGE (Pointer) */
    0xa1, 0x00,                    /*   COLLECTION (Physical) */
    0x05, 0x09,                    /*     USAGE_PAGE (Button) */
    0x19, 0x01,                    /*     USAGE_MINIMUM (Button 1) */
    0x29, 0x03,                    /*     USAGE_MAXIMUM (Button 3) */
    0x15, 0x00,                    /*     LOGICAL_MINIMUM (0) */
    0x25, 0x01,                    /*     LOGICAL_MAXIMUM (1) */
    0x95, 0x03,                    /*     REPORT_COUNT (3) */
    0x75, 0x01,                    /*     REPORT_SIZE (1) */
    0x81, 0x02,                    /*     INPUT (Data,Var,Abs) */
    0x95, 0x01,                    /*     REPORT_COUNT (1) */
    0x75, 0x05,                    /*     REPORT_SIZE (5) */
    0x81, 0x01,                    /*     INPUT (Cnst,Ary,Abs) */
    0x05, 0x01,                    /*     USAGE_PAGE (Generic Desktop) */
    0x09, 0x30,                    /*     USAGE (X) */
    0x09, 0x31,                    /*     USAGE (Y) */
    0x15, 0x81,                    /*     LOGICAL_MINIMUM (-127) */
    0x25, 0x7f,                    /*     LOGICAL_MAXIMUM (127) */
    0x75, 0x08,                    /*     REPORT_SIZE (8) */
    0x95, 0x02,                    /*     REPORT_COUNT (2) */
    0x81, 0x06,                    /*     INPUT (Data,Var,Rel) */
    0xc0,                          /*     END_COLLECTION */
    0xc0                           /* END_COLLECTION */
};

const hcc_u8 mou_config_descriptor[] = {
  USB_FILL_CFG_DESC(9+3+9+9+7, 1, 1, 4, CFGD_ATTR_SELF_PWR, 0),
  USB_FILL_OTG_DESC(1, 1),
  USB_FILL_IFC_DESC(MOU_IFC_INDEX, 0, 1, 0x3, 0x1, 0x2, 5), /* HID, boot, mouse (3/1/2) */
  USB_FILL_HID_DESC(9, 0x0100, 0x0, 1, 0x22, sizeof(mou_report_descriptor)),
  USB_FILL_EP_DESC(0x1, 1, 0x3, 8, 0x20),
};

/*************************************************************************/
/********************************** Generic HID **************************/
#define GEH_IFC_INDEX   0u
#define GEH_VENDOR_ID   0xc1cau
#define GEH_PRODUCT_ID  (3)
#define GEH_DEVICE_REL_NUM  0x0

const hcc_u8 geh_config[44] = { 44, 0x3, 'D',0, 'e',0, 'f',0, 'a',0, 'u',0
                  , 'l',0, 't',0, ' ',0, 'c',0, 'o',0, 'n',0, 'f',0, 'i',0
                  , 'g',0, 'u',0, 'r',0, 'a',0, 't',0, 'i',0, 'o',0, 'n',0};
const hcc_u8 geh_interface[18] = { 18, 0x3, 'H',0, 'I',0, 'D',0, '-',0, 'L',0
                  , 'E',0, 'D',0, 's',0 };
const hcc_u8 geh_serail_number[10] = { 10, 0x3, 'V',0, '1',0, '.',0, '0',0};
const hcc_u8 geh_product[76] = { 76, 0x3, 'U',0, 'S',0, 'B',0, ' ',0, 'H',0
                  , 'I',0, 'D',0, ' ',0, 'L',0, 'E',0, 'D',0, ' ',0, 'd',0
                  , 'e',0, 'm',0, 'o',0, ' ',0, 'f',0, 'o',0, 'r',0, ' ',0
                  , 'H',0, 'C',0, '9',0, 'S',0, '0',0, '8',0, 'J',0, 'M',0
                  , ' ',0, 'd',0, 'e',0, 'v',0, 'i',0, 'c',0, 'e',0, 's',0};

const hcc_u8 * geh_string_descriptors[] = {
  string_descriptor0, str_manufacturer, geh_product, geh_serail_number
  , geh_config, geh_interface
};

const hcc_u8 geh_device_descriptor[] = {
  USB_FILL_DEV_DESC(0x0101, 0, 0, 0, EP0_PACKET_SIZE, GEH_VENDOR_ID, GEH_PRODUCT_ID
      , GEH_DEVICE_REL_NUM, 1, 2, 3, 1)
};

const hcc_u8 geh_report_descriptor[46] = {
    0x06, 0x00, 0xff,              // USAGE_PAGE (Vendor Defined Page 1)
    0x09, 0x01,                    // USAGE (Vendor Usage 1)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x05, 0x08,                    //   USAGE_PAGE (LEDs)
    0x09, 0x4b,                    //   USAGE (Generic Indicator)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x95, 0x04,                    //   REPORT_COUNT (4)
    0x91, 0x02,                    //   OUTPUT (Data,Var,Abs)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x95, 0x04,                    //   REPORT_COUNT (4)
    0x91, 0x03,                    //   OUTPUT (Cnst,Var,Abs)
    0x05, 0x09,                    //   USAGE_PAGE (Button)
    0x19, 0x01,                    //   USAGE_MINIMUM (Button 1)
    0x29, 0x02,                    //   USAGE_MAXIMUM (Button 2)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x95, 0x02,                    //   REPORT_COUNT (2)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x95, 0x06,                    //   REPORT_COUNT (6)
    0x81, 0x03,                    //   INPUT (Cnst,Var,Abs)
    0xc0                           // END_COLLECTION
};

const hcc_u8 geh_config_descriptor[] = {
  USB_FILL_CFG_DESC(9+3+9+9+7, 1, 1, 4, CFGD_ATTR_SELF_PWR, 0),
  USB_FILL_OTG_DESC(1, 1),
  USB_FILL_IFC_DESC(GEH_IFC_INDEX, 0, 1, 0x3, 0x0, 0x0, 5),  /* (HID, none, none) */
  USB_FILL_HID_DESC(9, 0x0100, 0x0, 1, 0x22, sizeof(geh_report_descriptor)),
  USB_FILL_EP_DESC(0x1, 1, 0x3, 8, 0x20),
};

descriptor_info_t di;
/*************************************************************************/
/* Default mode at startup is keyboard mode. */
static device_mode_t device_mode=dm_kbd;

/*****************************************************************************
 * Set wich USB configuration application wants to use.
 ****************************************************************************/
void set_mode(device_mode_t mode)
{
  device_mode=mode;
}

/*****************************************************************************
 * HID reset event handler. 
 ****************************************************************************/
void got_usb_reset(void)
{
  /* empty */
}

/*****************************************************************************
 * Return the HID descriptor for the selected device.
 ****************************************************************************/
descriptor_info_t *get_hid_descriptor(void)
{
  switch(device_mode)
  {
  case dm_kbd:
    di.start_addr=(void *)(kbd_config_descriptor+9+3+9);
    di.size=9;    
    break;
  case dm_mouse:
    di.start_addr=(void *)(mou_config_descriptor+9+3+9);
    di.size=9;    
    break;  
  case dm_generic:
    di.start_addr=(void *)(geh_config_descriptor+9+3+9);
    di.size=9;    
    break;  
  }
  return(&di);
}

/*****************************************************************************
 * Return the report descriptor for the selected device.
 ****************************************************************************/
descriptor_info_t *get_report_descriptor(void)
{
  switch(device_mode)
  {
  case dm_kbd:
    di.start_addr=(void *)kbd_report_descriptor;
    di.size=sizeof(kbd_report_descriptor);
    break;
  case dm_mouse:
    di.start_addr=(void *)mou_report_descriptor;
    di.size=sizeof(mou_report_descriptor);    
    break;  
  case dm_generic:
    di.start_addr=(void *)geh_report_descriptor;
    di.size=sizeof(geh_report_descriptor);
    break;  
  }        
  return(&di);
}

/*****************************************************************************
 * Return the physical descriptor for the selected device.
 ****************************************************************************/
descriptor_info_t *get_physical_descriptor(hcc_u8 id)
{
  switch(device_mode)
  {
  case dm_kbd:
  case dm_mouse:
  case dm_generic:
    return(0);
  }    
  /* Can newer get here. */
  id=0;
  return(0);
}


/*****************************************************************************
 * Return the USB device descriptor.
 ****************************************************************************/
void* get_device_descriptor(void)
{
  switch(device_mode)
  {
  case dm_kbd:
    return((void *)kbd_device_descriptor);
    break;
  case dm_mouse:
    return((void *)mou_device_descriptor);
    break;  
  case dm_generic:
    return((void *)geh_device_descriptor);
    break;  
  }
  return(0);
}

/*****************************************************************************
 * Return !=0 if the specified index is the index of a configuration.
 ****************************************************************************/
hcc_u8 is_cfgd_index(hcc_u16 cndx)
{
  switch(device_mode)
  {
  case dm_kbd:
  case dm_mouse:
  case dm_generic:
    return((hcc_u8)(cndx < 2 ? 1u : 0u));
    break;  
  }    
  /* Can newer get here. Line added to avoid compiler warnings. */
  return(0);
}

/*****************************************************************************
 * Return the selected configuration descriptor.
 ****************************************************************************/
void *get_cfg_descriptor(hcc_u8 cndx)
{
  switch(device_mode)
  {
  case dm_kbd:
    return((void *)kbd_config_descriptor);
    break;
  case dm_mouse:
    return((void *)mou_config_descriptor);
    break;  
  case dm_generic:
    return((void *)geh_config_descriptor);
    break;  
  }
  /* Can newer get here. */
  cndx=0;
  return(0);
}

/*****************************************************************************
 * Return !=0 if a string descriptor exist whit the specified index.
 ****************************************************************************/
hcc_u8 is_str_index(hcc_u8 sndx)
{
  switch(device_mode)
  {
  case dm_kbd:
    return ((hcc_u8)(sndx < sizeof(kbd_string_descriptors)/sizeof(kbd_string_descriptors[0]) 
             ? 1u : 0u));
    break;
  case dm_mouse:
    return ((hcc_u8)(sndx < sizeof(mou_string_descriptors)/sizeof(mou_string_descriptors[0])
             ? 1u : 0u));
    break;  
  case dm_generic:
    return ((hcc_u8)(sndx < sizeof(geh_string_descriptors)/sizeof(geh_string_descriptors[0])
             ? 1u : 0u));
    break;
  }        
  return(0);
}

/*****************************************************************************
 * Get the specified string descriptor.
 ****************************************************************************/
void *get_str_descriptor(hcc_u8 sndx)
{
  switch(device_mode)
  {
  case dm_kbd:
    return((void*)kbd_string_descriptors[sndx]);
  case dm_mouse:
    return ((void*)mou_string_descriptors[sndx]);
  case dm_generic:
    return ((void*)geh_string_descriptors[sndx]);
  }            
  /* Can newer get here. Line added to avoid compiler warnings. */  
  return(0);
}

/*****************************************************************************
 * Return !=0 if the secified interface exist.
 ****************************************************************************/
hcc_u8 is_ifc_ndx(hcc_u8 cndx, hcc_u8 indx, hcc_u8 iset)
{ 
  switch(device_mode)
  {
  case dm_kbd:
  case dm_mouse:
  case dm_generic:
    if (cndx != 1 || iset !=0)
    {
      return(0); 
    }
    return((hcc_u8)(indx < 1 ? 1 : 0));
  }        
  return(0);
}

/*****************************************************************************
 * Return !=0 if the specified endpoint exist.
 ****************************************************************************/
hcc_u8 is_ep_ndx(hcc_u8 cndx, hcc_u8 indx, hcc_u8 iset, hcc_u8 endx)
{
  switch(device_mode)
  {
  case dm_kbd:
  case dm_mouse:
  case dm_generic:
    if (cndx != 1 || indx !=0  || iset !=0)
    {
      return(0); 
    }
    return((hcc_u8)(endx < 1 ? 1 : 0));
  }  
  return(0);
}

/*****************************************************************************
 * Return the endpoint descriptor of the specified endpoint.
 ****************************************************************************/
void *get_ep_descriptor(hcc_u8 cndx, hcc_u8 indx, hcc_u8 iset, hcc_u8 endx)
{
  switch(device_mode)
  {
  case dm_kbd:
    return((void*)(kbd_config_descriptor+9+3+9+9));
  case dm_mouse:
    return((void*)(mou_config_descriptor+9+3+9+9));
  case dm_generic:
    return((void*)(geh_config_descriptor+9+3+9+9));
  }
  /* Can newer get here. */
  cndx=indx=iset=endx=0;  
  return(0);
}

/*****************************************************************************
 * Return packet buffer of the specified endpoint.
 ****************************************************************************/
void *get_ep_rx_buffer(hcc_u8 ep, hcc_u8 buf)
{
  switch(ep)
  {
  case 0:
    if (!buf)
    {
      return(buf_ep00);
    }
    else
    {
      return(buf_ep01);    
    }
    break;
  }
  return(0);
}

#ifdef ON_THE_GO
callback_state_t usb_ep0_callback(void)
{
  callback_state_t r;   
  r=usb_ep0_hid_callback();
  if (r==clbst_error)
  {
    r=usb_ep0_otg_callback();
  }
  return(r);
}
#else
callback_state_t usb_ep0_callback(void)
{
  return(usb_ep0_hid_callback());
}
#endif

void usb_cfg_init(void)
{
  (void)usb_init();
}
/****************************** END OF FILE **********************************/
