#include <mc9S08jm60.h>

const char security @0xffbf=0xfe; //0xfe=visibile, 0xfc=invisibile

__interrupt  void SPI1(void);
__interrupt  void USB_status(void);
__interrupt  void timer1_overflowISR(void);
__interrupt  void timer2_overflowISR(void);
__interrupt  void AD_conversion_completeISR(void);
__interrupt  void UnimplementedISR(void)
{
asm
 {
 LDA _MCGSC
 }
}

typedef void(*tIsrFunc)(void);
const tIsrFunc _vect[] @0xffca={     /* Interrupt table */
   AD_conversion_completeISR,        /* ADC conversion 0xffca*/
   UnimplementedISR,                 /* KBI            0xffcc*/
   UnimplementedISR,                 /* SCI2 Transmit  0xffce*/
   UnimplementedISR,                 /* SCI2 Receive   0xffd0*/
   UnimplementedISR,                 /* SCI2 error     0xffd2*/
   UnimplementedISR,                 /* SCI1 Transmit  0xffd4*/
   UnimplementedISR,                 /* SCI1 Receive   0xffd6*/
   UnimplementedISR,                 /* SCI1 error     0xffd8*/
   timer2_overflowISR,               /* TPM2 Overflow  0xffda*/
   UnimplementedISR,                 /* TPM2 Channel 1 0xffdc*/
   UnimplementedISR,                 /* TPM2 Channel 0 0xffde*/
   timer1_overflowISR,               /* TPM1 Overflow  0xffe0*/
   UnimplementedISR,                 /* TPM1 Channel 5 0xffe2*/
   UnimplementedISR,                 /* TPM1 Channel 4 0xffe4*/
   UnimplementedISR,                 /* TPM1 Channel 3 0xffe6*/
   UnimplementedISR,                 /* TPM1 Channel 2 0xffe8*/
   UnimplementedISR,                 /* TPM1 Channel 1 0xffea*/
   UnimplementedISR,                 /* TPM1 Channel 0 0xffec*/
   UnimplementedISR,                 /* Reserved       0xffee*/
   USB_status,                       /* USB status     0xfff0*/
   UnimplementedISR,                 /* SPI2           0xfff2*/
   SPI1,                             /* SPI1           0xfff4*/
   UnimplementedISR,                 /* MCG Loss Lock  0xfff6*/
   UnimplementedISR,                 /* LowVoltageDet  0xfff8*/
   UnimplementedISR,                 /* IRQ            0xfffa*/
   UnimplementedISR                  /* SWI            0xfffc*/
   /*Startup  by default in library*/ /* Reset         0xfffe*/
   };