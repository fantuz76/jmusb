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
#include "target.h"



void _irq_restore (hcc_imask ip)
{
  if(ip)
  {
    /* Disable interrupts */
    asm ("sei");
  }
  else
  { 
    /* Enable interrupts */
    asm("cli");
  }
}

hcc_imask _irq_disable (void)
{
  hcc_u8 r;
  asm ("tpa;");     /* transfer CCR to A */
  asm ("sei;");     /* disable interrupts */
  asm ("and #8;");  /* mask I bit of CCR */
  asm ("sta r;");
  return(r);
}


/****************************** END OF FILE **********************************/
