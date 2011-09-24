#include <mc9S08jm60.h>



const byte NVOPT_INIT  @0x0000FFBF = 0x02;    // vector redirect, flash unsecure
const byte NVPROT_INIT @0x0000FFBD = 0xFA;    // 0xFC00-0xFFFF are protected 



extern void _Startup(void);



const char security @0xffbf=0xfe; //0xfe=visibile, 0xfc=invisibile

__interrupt  void SPI1(void);

__interrupt  void usb_it_handler(void);
__interrupt  void AD_conversion_completeISR(void);
__interrupt  void timer2_captureISR(void);
__interrupt void timer1_overflowISR(void); //timer 100 us per relay
__interrupt  void UnimplementedISR(void)
{
asm
 {
 LDA _MCGSC
 }
}

void (* volatile const _UserEntry[])()@0xFABC={
  0x9DCC,             // asm NOP(9D), asm JMP(CC)
  _Startup
};

// Interrupt Vector Originale    
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
   UnimplementedISR,                 /* TPM2 Overflow  0xffda*/
   UnimplementedISR,                 /* TPM2 Channel 1 0xffdc*/
   timer2_captureISR,                /* TPM2 Channel 0 0xffde*/
   timer1_overflowISR,               /* TPM1 Overflow  0xffe0*/
   UnimplementedISR,                 /* TPM1 Channel 5 0xffe2*/
   UnimplementedISR,                 /* TPM1 Channel 4 0xffe4*/
   UnimplementedISR,                 /* TPM1 Channel 3 0xffe6*/
   UnimplementedISR,                 /* TPM1 Channel 2 0xffe8*/
   UnimplementedISR,                 /* TPM1 Channel 1 0xffea*/
   UnimplementedISR,                 /* TPM1 Channel 0 0xffec*/
   UnimplementedISR,                 /* Reserved       0xffee*/
   usb_it_handler,                   /* USB status     0xfff0*/
   UnimplementedISR,                 /* SPI2           0xfff2*/
   SPI1,                             /* SPI1           0xfff4*/
   UnimplementedISR,                 /* MCG Loss Lock  0xfff6*/
   UnimplementedISR,                 /* LowVoltageDet  0xfff8*/
   UnimplementedISR,                 /* IRQ            0xfffa*/
   UnimplementedISR                  /* SWI            0xfffc*/
   /*Startup  by default in library*/ /* Reset         0xfffe*/
   };
   
   
// redirect vector 0xFFC0-0xFFFD to 0xFBC0-0xFBFD
void (* volatile const _Usr_Vector[])()@0xFBC4= {
    UnimplementedISR,             // Int.no.29 RTC               (at FBC4) (at FFC4)
    UnimplementedISR,             // Int.no.28 IIC               (at FBC6) (at FFC6)
    AD_conversion_completeISR,    // Int.no.27 ACMP              (at FBC8) (at FFC8)
    UnimplementedISR,             // Int.no.26 ADC               (at FBCA) (at FFCA)
    UnimplementedISR,             // Int.no.25 KBI               (at FBCC) (at FFCC)
    UnimplementedISR,             // Int.no.24 SCI2 Transmit     (at FBCE) (at FFCE)
    UnimplementedISR,             // Int.no.23 SCI2 Receive      (at FBD0) (at FFD0)
    UnimplementedISR,             // Int.no.22 SCI2 Error        (at FBD2) (at FFD2)
    UnimplementedISR,             // Int.no.21 SCI1 Transmit     (at FBD4) (at FFD4)
    UnimplementedISR,             // Int.no.20 SCI1 Receive      (at FBD6) (at FFD6)
    UnimplementedISR,             // Int.no.19 SCI1 error        (at FBD8) (at FFD8)
    UnimplementedISR,             // Int.no.18 TPM2 Overflow     (at FBDA) (at FFDA)
    UnimplementedISR,             // Int.no.17 TPM2 CH1          (at FBDC) (at FFDC)
    timer2_captureISR,            // Int.no.16 TPM2 CH0          (at FBDE) (at FFDE)
    timer1_overflowISR,           // Int.no.15 TPM1 Overflow     (at FBE0) (at FFE0)
    UnimplementedISR,             // Int.no.14 TPM1 CH5          (at FBE2) (at FFE2)
    UnimplementedISR,             // Int.no.13 TPM1 CH4          (at FBE4) (at FFE4)
    UnimplementedISR,             // Int.no.12 TPM1 CH3          (at FBE6) (at FFE6)
    UnimplementedISR,             // Int.no.11 TPM1 CH2          (at FBE8) (at FFE8)
    UnimplementedISR,             // Int.no.10 TPM1 CH1          (at FBEA) (at FFEA)
    UnimplementedISR,             // Int.no.9  TPM1 CH0          (at FBEC) (at FFEC)
    UnimplementedISR,             // Int.no.8  Reserved          (at FBEE) (at FFEE)
    usb_it_handler,               // Int.no.7  USB Statue        (at FBF0) (at FFF0)
    UnimplementedISR,             // Int.no.6  SPI2              (at FBF2) (at FFF2)
    SPI1,                         // Int.no.5  SPI1              (at FBF4) (at FFF4)
    UnimplementedISR,             // Int.no.4  Loss of lock      (at FBF6) (at FFF6)
    UnimplementedISR,             // Int.no.3  LVI               (at FBF8) (at FFF8)
    UnimplementedISR,             // Int.no.2  IRQ               (at FBFA) (at FFFA)
    UnimplementedISR,             // Int.no.1  SWI               (at FBFC) (at FFFC) 
};



#pragma CODE_SEG Bootloader_ROM

void Bootloader_Main(void);

void _Entry(void) {

  MCGC2=0x37;
  while (MCGSC_OSCINIT == 0)
    ;
    
  MCGC1=0x00;
    /* wait for mode change to be done */
  while (MCGSC_IREFST != 0)
    ;

  MCGC3=0x43;
  while(MCGSC_PLLST != 1)
    ;
  while(MCGSC_LOCK != 1)
    ;

  //----portA--------
  PTADD=0x03; //pulsanti ed abilitazione  //mofificato il 22-07-2011
  PTAD=0x03;                              //mofificato il 22-07-2011

  PTADD=0; //pulsanti ed abilitazione


  // Flash clock
  FCDIV=0x4E;                     // PRDIV8=1; DIV[5:0]=14, flash clock should be 150-200kHz
                                  // bus clock=24M, flash clock=fbus/8/(DIV[5:0]+1) 
                                 // bus clock=24M, flash clock=fbus/8/(DIV[5:0]+1) 
  if((PTAD_PTAD0==0) && (PTAD_PTAD1==0) )
  { 
    SOPT1 = 0x20;                   // disable COP only if bootloader mod is requested
    // PTG0 is pressed
    USBCTL0=0x44;
    Bootloader_Main();            // Bootloader mode
  }
  else
    asm JMP _UserEntry;           // Enter User mode
}

#pragma CODE_SEG default

