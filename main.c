#include <xc.h>
#include <sys/attribs.h>


// DEVCFG3
// USERID = No Setting
 

// DEVCFG2
#pragma config FPLLIDIV = 0        // System PLL Input Divider (1x Divider)
#pragma config FPLLRNG = RANGE_5_10_MHZ// System PLL Input Range (5-10 MHz Input)
#pragma config FPLLICLK = PLL_FRC       // System PLL Input Clock Selection (FRC is input to the System PLL)
#pragma config FPLLMULT = MUL_32       // System PLL Multiplier (PLL Multiply by 128)
#pragma config FPLLODIV = 1       // System PLL Output Clock Divider (2x Divider) ==> 64 MHz SYSCLK

// DEVCFG1
#pragma config FNOSC = SPLL             // Oscillator Selection Bits (SPLL))
#pragma config DMTINTV = WIN_127_128    // DMT Count Window Interval (Window/Interval value is 127/128 counter value)
#pragma config FSOSCEN = ON             // Secondary Oscillator Enable (Enable Secondary Oscillator)
#pragma config IESO = ON                // Internal/External Switch Over (Enabled)
#pragma config POSCMOD = OFF            // Primary Oscillator Configuration (Primary osc disabled)
#pragma config OSCIOFNC = OFF           // CLKO Output Signal Active on the OSCO Pin (Disabled)
#pragma config FCKSM = CSECME           // Clock Switching and Monitor Selection (Clock Switch Enabled, FSCM Enabled)
#pragma config WDTPS = PS1048576        // Watchdog Timer Postscaler (1:1048576)
#pragma config WDTSPGM = STOP           // Watchdog Timer Stop During Flash Programming (WDT stops during Flash programming)
#pragma config WINDIS = NORMAL          // Watchdog Timer Window Mode (Watchdog Timer is in non-Window mode)
#pragma config FWDTEN = ON              // Watchdog Timer Enable (WDT Enabled)
#pragma config FWDTWINSZ = WINSZ_25     // Watchdog Timer Window Size (Window size is 25%)
#pragma config DMTCNT = DMT31           // Deadman Timer Count Selection (2^31 (2147483648))
#pragma config FDMTEN = ON              // Deadman Timer Enable (Deadman Timer is enabled)

// DEVCFG0
#pragma config DEBUG = ON              // Background Debugger Enable (Debugger is disabled)
#pragma config JTAGEN = ON              // JTAG Enable (JTAG Port Enabled)
#pragma config ICESEL = ICS_PGx3        // ICE/ICD Comm Channel Select (Communicate on PGEC1/PGED1)
#pragma config TRCEN = ON               // Trace Enable (Trace features in the CPU are enabled)
#pragma config BOOTISA = MIPS32         // Boot ISA Selection (Boot code and Exception code is MIPS32)
#pragma config FSLEEP = OFF             // Flash Sleep Mode (Flash is powered down when the device is in Sleep mode)
#pragma config DBGPER = PG_ALL          // Debug Mode CPU Access Permission (Allow CPU access to all permission regions)
#pragma config SMCLR = MCLR_NORM        // Soft Master Clear Enable (MCLR pin generates a normal system Reset)
#pragma config SOSCGAIN = GAIN_2X       // Secondary Oscillator Gain Control bits (2x gain setting)
#pragma config SOSCBOOST = ON           // Secondary Oscillator Boost Kick Start Enable bit (Boost the kick start of the oscillator)
#pragma config POSCGAIN = GAIN_LEVEL_3  // Primary Oscillator Gain Control bits (Gain Level 3 (highest))
#pragma config POSCBOOST = ON           // Primary Oscillator Boost Kick Start Enable bit (Boost the kick start of the oscillator)
#pragma config EJTAGBEN = NORMAL        // EJTAG Boot Enable (Normal EJTAG functionality)

void __ISR (_OUTPUT_COMPARE_1_VECTOR, IPL7AUTO) OC1_IntHandler (void){

    LATDbits.LATD5 = 0;
    IFS0bits.OC1IF = 0; // Clear the OC1 interrupt flag
}

int main(void) {

    TRISDbits.TRISD5 = 0;   //LED1 as output
    LATDbits.LATD5 = 0;
    
    
    RPA0Rbits.RPA0R = 0; //unMap RPA0
    TRISAbits.TRISA0 = 0;   //pin 13 as output
    LATAbits.LATA0 = 0;
    
    OC1CONbits.ON = 0; //turn off during setup
    OC1CONbits.OC32 = 0; //enable 16 bit comparaison
    OC1CONbits.OCM = 3; // Initialize OCx pin high; compare event forces OCx pin low
    OC1CONbits.OCTSEL = 0; // select timer 2, CFGCON=0
    //OC1R = 30; // Initialize primary Compare Register
    OC1RS = 500; // Initialize secondary Compare Register
    PR2 = 1000; // set period
    
    RPA0Rbits.RPA0R = 5; //Map OC1 on RPA0 (pin 13)
    T2CONbits.T32 = 0;
    
    //configure OC1 interrupt
    IEC0bits.OC1IE = 0; //disable interupt OC1
    IFS0bits.OC1IF = 0; // Clear the OC1 interrupt flag
    IPC1bits.OC1IP = 7; // Set OC1 interrupt priority to 7 (highest) bit26:28)
    IPC1bits.OC1IS = 3; // subpriority to 3 (max) bit 24:25
    IEC0bits.OC1IE = 1; //enable interupt OC1
    __builtin_enable_interrupts();    /* Set the CP0 Status IE bit to turn on interrupts globally */
            
    T2CONbits.ON = 1; //turn on timer 2
    OC1CONbits.ON = 1; //turn on OC1
        
    int i =0;
    
   while(1){
        for(i = 0; i<1000000; i++);    
        LATDbits.LATD5 = 1;
        for(i = 0; i<1000000; i++);  
        LATDbits.LATD5 = 0;
   }
   return 0;
}


