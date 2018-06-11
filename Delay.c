#include <xc.h>
#include <cp0defs.h>
#include <sys/attribs.h>

#define CLK_FREQUENCY 64000000
#define ms_SCALE   (CLK_FREQUENCY/2000)
#define us_SCALE   (CLK_FREQUENCY/2000000)


/***********************************************************
 *   Millisecond Delay function using the Count register 
 *   in coprocessor 0 in the MIPS core.
 */
void DelayMs(unsigned long int msDelay )
{
      register unsigned int startCntms = _CP0_GET_COUNT();
      register unsigned int waitCntms = msDelay * ms_SCALE;
      
      while( _CP0_GET_COUNT() - startCntms < waitCntms );
}

/***********************************************************
 *   Microsecond Delay function using the Count register 
 *   in coprocessor 0 in the MIPS core.
 */
void DelayUs(unsigned long int usDelay )
{
      register unsigned int startCntms = _CP0_GET_COUNT();
      register unsigned int waitCntms = usDelay * us_SCALE;
     
      LATDbits.LATD5 = 1;
      while( _CP0_GET_COUNT() - startCntms < waitCntms );
      LATDbits.LATD5 = 0;
}


//test with timer 2
void setup_delay_timer2()
{
    /*Configure Timer 2*/
        T2CONbits.ON = 0;       //turn off timer 2
        T2CONbits.T32 = 0;      //16bits timer
        T2CONbits.TCS = 0;      //Internal peripheral clock
        T2CONbits.TGATE = 0;    //Gated time accumulation is disabled => select PBCLK2 like clock source
        T2CONbits.TCKPS = 0;    // 1:1 prescale value
        
        /*Unlock sequence to modify PB2DIV*/
        __builtin_disable_interrupts();
        SYSKEY = 0xAA996655;
        SYSKEY = 0x556699AA;
        __builtin_enable_interrupts();    /* Set the CP0 Status IE bit to turn on interrupts globally */
        
        PB2DIVbits.PBDIV = 0;   //divided by 1
        PB2DIVbits.ON = 1;      //Output clock is enabled
        INTCONbits.MVEC = 1;    //configure in multiple vector mode
        
        //configure Timer 2 interrupt
        IEC0bits.T2IE = 0;      //disable interrupt T2                   @
        IFS0bits.T2IF = 0;      // Clear the T2 interrupt flag
        IPC2bits.T2IP = 7;      // Set T2 interrupt priority to 7 (highest) bit26:28)
        IPC2bits.T2IS = 3;      // subpriority to 3 (max) bit 24:25
        IEC0bits.T2IE = 1;      //enable interrupt T2
        PRISSbits.PRI7SS = 1;   //Interrupt with a priority level of 7 uses Shadow Set 1
}

//test with core timer interrupt
//http://ww1.microchip.com/downloads/en/DeviceDoc/50002509B.pdf
void delay(int time){
   
    __builtin_disable_interrupts();
    // Configure the core timer see Section 3.3 // clear the CP0 Count register
    _CP0_SET_COUNT(0);
    // set up the period in the CP0 Compare register 
    //_CP0_SET_COMPARE();
    
    // halt core timer and program at a debug breakpoint
    //_CP0_BIC_DEBUG(_CP0_DEBUG_COUNTDM_MASK);
     

    // Set up core timer interrupt
     // clear core timer interrupt flag
     IFS0CLR = _IFS0_CTIF_MASK;
     // set core time interrupt priority of 2
     IPC0CLR = _IPC0_CTIP_MASK;
     IPC0SET = (2 << _IPC0_CTIP_POSITION);
     // set core time interrupt subpriority of 0
     IPC0CLR = _IPC0_CTIS_MASK;
     IPC0SET = (0 << _IPC0_CTIS_POSITION);
     // enable core timer interrupt
     IEC0CLR = _IEC0_CTIE_MASK;
     IEC0SET = (1 << _IEC0_CTIE_POSITION);
     // set the CP0 Cause register Interrupt Vector bit
                   //cause_val = _CP0_GET_CAUSE();
                   //cause_val |= _CP0_CAUSE_IV_MASK;
                   //_CP0_SET_CAUSE(cause_val);

                   // enable global interrupts
                   __builtin_enable_interrupts();
      
        
        
}
