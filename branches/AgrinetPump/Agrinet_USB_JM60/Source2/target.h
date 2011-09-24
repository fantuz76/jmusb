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
#ifndef _TARGET_H_
#define _TARGET_H_

#include "hcc_types.h"
#include "derivative.h"

extern void _irq_restore (hcc_imask ip);
extern hcc_imask _irq_disable (void);




  /* Include the derivative-specific header file */
#define BIT0 (1u<<0)
#define BIT1 (1u<<1)
#define BIT2 (1u<<2)
#define BIT3 (1u<<3)
#define BIT4 (1u<<4)
#define BIT5 (1u<<5)
#define BIT6 (1u<<6)
#define BIT7 (1u<<7)

#endif
/****************************** END OF FILE **********************************/
