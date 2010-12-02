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
 #include "hid.h"
 #include "usb-drv/usb.h"

/****************************************************************************
 ************************** Macro definitions *******************************
 ***************************************************************************/
#define MAX_NO_OF_REPORTS 2
#define MAX_REPORT_LENGTH 8

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

#define MIN(a,b)     ((a) < (b) ? (a) : (b))

/****************************************************************************
 ************************** Local type definitions. *************************
 ***************************************************************************/
typedef struct {
  hcc_u8 used;
  hcc_u8 pending;  
  hcc_u8 buffer[MAX_REPORT_LENGTH];  
  hid_report_type type;
  hcc_u8 id;
  hcc_u8 size;
} report_t;

/****************************************************************************
 ************************** Function predefinitions. ************************
 ***************************************************************************/
callback_state_t set_report_state(void);

/****************************************************************************
 ************************** Global variables ********************************
 ***************************************************************************/
/* none */

/****************************************************************************
 ************************** Module variables ********************************
 ***************************************************************************/
static hcc_u16 hid_idle_time;
static hcc_u8 hid_ifc_number;
static hcc_u8 duration_tx;
static report_t *pendign_rep;
static report_t reports[MAX_NO_OF_REPORTS];

/*****************************************************************************
 * Define new report for the HID driver.
 ****************************************************************************/
hcc_u8 hid_add_report(hid_report_type type, hcc_u8 id, hcc_u8 size)
{
  int x;
  for(x=0;x<sizeof(reports)/(sizeof(reports[0])); x++)
  {
    if (reports[x].used==0)
    {
      reports[x].used=1;    
      reports[x].type=type;
      reports[x].id=id;
      reports[x].size=size;
      reports[x].pending=0;      
      return((hcc_u8)x);
    }    
  }
  return((hcc_u8)-1);
}

/*****************************************************************************
 * Update report data.
 ****************************************************************************/
void hid_write_report(hcc_u8 r, hcc_u8 *data)
{
  int x;
  for(x=0; x < reports[r].size; x++)
  {  
    reports[r].buffer[x]=data[x];
  }
  reports[r].pending=1;
}

/*****************************************************************************
 * Read report data.
 ****************************************************************************/
void hid_read_report(hcc_u8 r, hcc_u8 *data)
{
  int x;
  for(x=0; x < reports[r].size; x++)
  {  
    data[x]=reports[r].buffer[x];
  }
  reports[r].pending=0;
}

hcc_u8 hid_report_pending(hcc_u8 r)
{
  return((hcc_u8)(reports[r].pending ? 1 : 0));
}

/*****************************************************************************
 * hid_get_idle_time
 ****************************************************************************/
hcc_u16 hid_get_idle_time(void)
{
  return(hid_idle_time);
}

/*****************************************************************************
 * duration2ms
 ****************************************************************************/
static hcc_u16 duration2ms(hcc_u16 duration)
{
  if ((duration & 0xff00) == 0)
  {
    return(0);
  }

  return((hcc_u16)((duration & 0xff) << 4));
}

/*****************************************************************************
 * ms2duration
 ****************************************************************************/
static hcc_u8 ms2duration(hcc_u16 ms)
{
  return((hcc_u8)(ms ? (0x1000 | (ms>>4)) : 0));
}

/*****************************************************************************
 * USB callback function. Is called by the USB driver if an USB error event
 * occurs.
 ****************************************************************************/
void usb_bus_error_event(void)
{
  /* empty */
}

/*****************************************************************************
 * USB callback function. Is called by the USB driver if an USB wakeup event
 * occurs.
 ****************************************************************************/
void usb_wakeup_event(void)
{
  /* empty */
}

/*****************************************************************************
 * USB callback function. Is called by the USB driver if an USB suspend event
 * occurs.
 ****************************************************************************/
void usb_suspend_event(void)
{
  /* empty */
}

/*****************************************************************************
 * USB callback function. Is called by the USB driver if an USB reset event
 * occurs.
 ****************************************************************************/
void usb_reset_event(void)
{
  int x;
  for(x=0; x<sizeof(reports)/sizeof(reports[0]); x++)
  {
    if(reports[x].used)
    {
      reports[x].pending=0;
    }
  }           
  got_usb_reset();
}


/*****************************************************************************
 * USB callback function. Is called by the USB driver when class specific 
 * IN/OUT request ended on EP0.
 ****************************************************************************/
callback_state_t set_report_state(void)
{
  if (pendign_rep->type==rpt_in)
  {
    pendign_rep->pending=0;
  }
  else if (pendign_rep->type==rpt_out)
  {
    pendign_rep->pending=1;
  }
  pendign_rep=0;
  return(clbst_ok);
}


/*****************************************************************************
 * USB callback function. Is called by the USB driver if a non standard
 * request is received from the HOST. This callback extends the known
 * requests by masstorage related ones.
 ****************************************************************************/
callback_state_t usb_ep0_hid_callback(void)
{
  hcc_u8 *pdata=usb_get_rx_pptr(0);

  callback_state_t r=(STP_REQU_TYPE(pdata) & (1u<<7)) ? clbst_in: clbst_out;

  if (STP_INDEX(pdata) == hid_ifc_number)
  { /* if the host wants to read some data */
    report_t *rep=0;
    if (r==clbst_in)
    { /* is this a standard request to an interface ? */
      if (STP_REQU_TYPE(pdata) == ((1<<7) | (0<<5) | 1))
      { /* the only standard request we support is get_descriptor */
        descriptor_info_t *dp=0;
        if (STP_REQUEST(pdata) == USBRQ_GET_DESCRIPTOR)
        {
          switch(STP_VALUE(pdata) & 0xff00u)
          {
          case GHIDD_HID_DESCRIPTOR << 8:
            dp=get_hid_descriptor();
            break;
          case GHIDD_REPORT_DESCRIPTOR <<8 :
            dp=get_report_descriptor();
            break;
          case GHIDD_PHYSICAL_DESCRIPTOR <<8 :
            dp=get_physical_descriptor((hcc_u8)STP_VALUE_LO(pdata));
            break;
          }
          
          if (dp)
          {
            usb_send(0                  /* Ep index. */
                   , (void *) 0         /* EP callback function. */
                   , dp->start_addr     /* Pointer to TX buffer. */
                   , dp->size           /* Sizeof TX buffer. */
                   , STP_LENGTH(pdata));/* Requested length. */
          }
        }
        else
        {
          r=clbst_error;
        }
      }
      /* is this a class specific in request to an interface? */
      else if (STP_REQU_TYPE(pdata) == ((1<<7) | (1<<5) | 1))
      {
        int x;        
        switch(STP_REQUEST(pdata))
        {
        case HIDRQ_GET_REPORT:
          switch(STP_VALUE(pdata) & 0xff00)
          {
          case 1<<8:  /* Report type input */
            for(x=0; x<sizeof(reports)/sizeof(reports[0]); x++)
            {
              if(reports[x].used 
                 && reports[x].type==rpt_in
                 && reports[x].id==STP_VALUE_LO(pdata))
              {
                rep=reports+x;
              }
            }     
            break;
          case 2<<8:  /* Report type output */
            for(x=0; x<sizeof(reports)/sizeof(reports[0]); x++)
            {
              if(reports[x].used 
                 && reports[x].type==rpt_out
                 && reports[x].id==STP_VALUE_LO(pdata))
              {
                rep=reports+x;
              }
            }
            break;
          case 3<<8:  /* Report type feature */
            for(x=0; x<sizeof(reports)/sizeof(reports[0]); x++)
            {
              if(reports[x].used 
                 && reports[x].type==rpt_feature
                 && reports[x].id==STP_VALUE_LO(pdata))
              {
                rep=reports+x;              
              }
            }
            break;
          }

          if (rep)
          {
            pendign_rep=rep;
            usb_send(0, set_report_state
                      , rep->buffer
                      , rep->size
                      , STP_LENGTH(pdata));
          }
          else
          {
            r=clbst_error;
          }          
          break;
        case HIDRQ_GET_IDLE:
          duration_tx=ms2duration(hid_idle_time);
          usb_send(0, (void*)0, &duration_tx, 1, STP_LENGTH(pdata));
          break;
        default:
        case HIDRQ_GET_PROTOCOL:
          r=clbst_error;
          break;
        }
      }
      else
      {
        r=clbst_error;
      }
    }
    else /* Host wants to send data to us. */
    { /* Is this a class specific OUT request to an interface? */
      if ((STP_REQU_TYPE(pdata) == ((0<<7) | (1<<5) | 1)))
      {
        int x;
        switch(STP_REQUEST(pdata))
        {
        case HIDRQ_SET_IDLE:
          hid_idle_time=duration2ms(STP_VALUE(pdata));
          break;
        case HIDRQ_SET_REPORT:
          switch(STP_VALUE(pdata) & 0xff00)
          {
          case 1<<8:  /* Report type input */
            for(x=0; x<sizeof(reports)/sizeof(reports[0]); x++)
            {
              if(reports[x].used 
                 && reports[x].type==rpt_in
                 && reports[x].id==STP_VALUE_LO(pdata))
              {
                rep=reports+x;
              }
            }     
            break;
          case 2<<8:  /* Report type output */
            for(x=0; x<sizeof(reports)/sizeof(reports[0]); x++)
            {
              if(reports[x].used 
                 && reports[x].type==rpt_out
                 && reports[x].id==STP_VALUE_LO(pdata))
              {
                rep=reports+x;
              }
            }
            break;
          case 3<<8:  /* Report type feature */
            for(x=0; x<sizeof(reports)/sizeof(reports[0]); x++)
            {
              if(reports[x].used 
                 && reports[x].type==rpt_feature
                 && reports[x].id==STP_VALUE_LO(pdata))
              {
                rep=reports+x;
              }
            }
          }
          
          if (rep)
          {
            pendign_rep=rep;
            usb_receive(0, set_report_state
                         , rep->buffer
                         , (hcc_u32)MIN(STP_LENGTH(pdata), rep->size));
          }
          else
          {
            r=clbst_error;
          }          
          break;
        case HIDRQ_SET_PROTOCOL:
        default:        
          r=clbst_error;
          break;
        }
      }
      else
      {
        r=clbst_error;
      }
    }
  }
  else /* The request is not for our interface. Report error. */
  {
    r=clbst_error;
  }
  return(r);
}
/*****************************************************************************
 * HID_init
 ****************************************************************************/
void HID_init(hcc_u16 default_idle_time, hcc_u8 ifc_number)
{
  int x;
  hid_idle_time=default_idle_time;
  hid_ifc_number=ifc_number;
  pendign_rep=0;
  for(x=0;x<sizeof(reports)/(sizeof(reports[0])); x++)
  {
    reports[x].used=0;
    reports[x].pending=0;          
  }
}

/*****************************************************************************
 * HID_process
 ****************************************************************************/
void hid_process(void)
{
  int x;
  
  if (usb_get_state() != USBST_CONFIGURED)
  {
    return; 
  }
  
  for(x=0; x<sizeof(reports)/sizeof(reports[0]); x++)
  {
    if(reports[x].used 
       && reports[x].pending     
       && reports[x].type==rpt_in)
    {
      usb_send(HID_IT_EP_NDX, (void *)0
               , reports[x].buffer
               , reports[x].size
               , reports[x].size);
      while(usb_ep_is_busy(HID_IT_EP_NDX))
        ;
      reports[x].pending=0;
      break;
    }
  }         
}
