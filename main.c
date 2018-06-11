#include <xc.h>
#include <stdio.h>
#include <sys/attribs.h>
#include <cp0defs.h>
#include <sys/kmem.h> //to manipulate virtual/physical address
#include <string.h>

#include "UART.h"
#include "SPI.h"
#include "MPU-9250.h"
#include "Delay.h"
#include "Flash.h"
#include "Recording.h"



// DEVCFG3
// USERID = No Setting
 

// DEVCFG2
#pragma config FPLLIDIV = 0        // System PLL Input Divider (1x Divider)
#pragma config FPLLRNG = RANGE_5_10_MHZ// System PLL Input Range (5-10 MHz Input)
#pragma config FPLLICLK = PLL_FRC       // System PLL Input Clock Selection (FRC is input to the System PLL)
#pragma config FPLLMULT = MUL_32       // System PLL Multiplier (PLL Multiply by 128)
#pragma config FPLLODIV = 2       // System PLL Output Clock Divider (2x Divider) ==> 64 MHz SYSCLK

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
#pragma config JTAGEN = OFF              // JTAG Enable (JTAG Port Enabled)
#pragma config ICESEL = ICS_PGx3        // ICE/ICD Comm Channel Select (Communicate on PGEC1/PGED1)
#pragma config TRCEN = OFF               // Trace Enable (Trace features in the CPU are enabled)
#pragma config BOOTISA = MIPS32         // Boot ISA Selection (Boot code and Exception code is MIPS32)
#pragma config FSLEEP = OFF             // Flash Sleep Mode (Flash is powered down when the device is in Sleep mode)
#pragma config DBGPER = PG_ALL          // Debug Mode CPU Access Permission (Allow CPU access to all permission regions)
#pragma config SMCLR = MCLR_NORM        // Soft Master Clear Enable (MCLR pin generates a normal system Reset)
#pragma config SOSCGAIN = GAIN_2X       // Secondary Oscillator Gain Control bits (2x gain setting)
#pragma config SOSCBOOST = ON           // Secondary Oscillator Boost Kick Start Enable bit (Boost the kick start of the oscillator)
#pragma config POSCGAIN = GAIN_LEVEL_3  // Primary Oscillator Gain Control bits (Gain Level 3 (highest))
#pragma config POSCBOOST = ON           // Primary Oscillator Boost Kick Start Enable bit (Boost the kick start of the oscillator)
#pragma config EJTAGBEN = NORMAL        // EJTAG Boot Enable (Normal EJTAG functionality)


#define CLK_FREQUENCY 64000000

__uint8_t MODE = 0;
volatile __uint8_t ON_OFF = 0;
__uint8_t R_W_ON_OFF = 1;


void fastPerformance(){
    
        __builtin_mtc0(_CP0_CONFIG, _CP0_CONFIG_SELECT, _CP0_CONFIG | 0x00000003);//Instruction Pre-fetch cached (Recommended)
        //CHECONbits.ICHEEN = 1;//enable instruction cache
        //CHECONbits.PFMAWSEN = 1;//Add one more Wait State to flash address setup (suggested for higher system clock frequencies)
        //CHECONbits.PREFEN = 1;//Enable predictive prefetch for CPU instructions only
        CHECONbits.PFMWS = 2;//Two Wait states   recommended  60 MHz < SYSCLK  < 80 MHz <=> 2 wait state
        
        __builtin_enable_interrupts();    /* Set the CP0 Status IE bit to turn on interrupts globally */
        INTCONbits.MVEC = 1;    //configure in multiple vector mode interupt

}

void timer2Setup(){
    
    RPA0Rbits.RPA0R = 0; //unMap RPA0
    TRISAbits.TRISA0 = 0;   //pin 13 as output
    LATAbits.LATA0 = 0;
    TRISCbits.TRISC10 = 0;//D5 as outpout

    OC1CONbits.ON = 0; //turn off during setup
    OC1CONbits.OC32 = 0; //enable 16 bit comparaison
    OC1CONbits.OCM = 6; // PWM mode pin fault disable
    OC1CONbits.OCTSEL = 0; // select timer 2, CFGCON=0

    OC1R = 0; // Initialize duty cycle
    OC1RS = 0; // 
    PR2 = 60000; // set period

    RPA0Rbits.RPA0R = 5; //Map OC1 on RPA0 (pin 13)

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
    PB2DIVbits.PBDIV = 0x7f;   //divided by 128
    PB2DIVbits.ON = 1;      //Output clock is enabled
    //INTCONbits.MVEC = 1;    //configure in multiple vector mode

    //configure OC1 interrupt
    IEC0bits.OC1IE = 0; //disable interrupt OC1
    IFS0bits.OC1IF = 0; // Clear the OC1 interrupt flag
    IPC1bits.OC1IP = 7; // Set OC1 interrupt priority to 7 (highest) bit26:28)
    IPC1bits.OC1IS = 3; // subpriority to 3 (max) bit 24:25
    IEC0bits.OC1IE = 1; //enable interrupt OC1

    //configure Timer 2 interrupt
    IEC0bits.T2IE = 0; //disable interrupt T2                   @
    IFS0bits.T2IF = 0; // Clear the T2 interrupt flag
    IPC2bits.T2IP = 2; // Set OC1 interrupt priority to 2
    IPC2bits.T2IS = 1; // subpriority to 1
    IEC0bits.T2IE = 1; //enable interrupt T2
    PRISSbits.PRI2SS = 1; //Interrupt with a priority level of 7 uses Shadow Set 1

    T2CONbits.ON = 0; //turn off timer 2
    OC1CONbits.ON = 0; //turn off OC1
}  

void confExternalInterrupt1(){
    
    TRISBbits.TRISB14 = 1;//B14 as input
    INT1R = 0; //map INT1 on RPB14
    
    //configure INT1 interrupt
    IEC0bits.INT1IE = 0; // disable interrupt INT1
    IFS0bits.INT1IF = 0; // Clear the INT1 interrupt flag
    IPC2bits.INT1IP = 7; // Set interrupt priority to 7 (highest) bit26:28)
    IPC2bits.INT1IS = 3; // subpriority to 3 (max) bit 24:25
    PRISSbits.PRI7SS = 1;// Interrupt with a priority level of 7 uses Shadow Set 1
    INTCONbits.INT1EP = 0; // Configure to falling edge
    IEC0bits.INT1IE = 1; // enable interrupt INT1
    
}

void confExternalInterrupt2(){
    
    //TRISGbits.TRISG8 = 1;//G8 as input
    TRISAbits.TRISA7 = 1;//G8 as input
    INT2R = 7; //map INT2 on G8
    
    //configure INT1 interrupt
    IEC0bits.INT2IE = 0; // disable interrupt INT1
    IFS0bits.INT2IF = 0; // Clear the INT1 interrupt flag
    IPC3bits.INT2IP = 6; // Set interrupt priority to 7 (highest) bit26:28)
    IPC3bits.INT2IS = 3; // subpriority to 3 (max) bit 24:25
    PRISSbits.PRI6SS = 1;// Interrupt with a priority level of 7 uses Shadow Set 1
    INTCONbits.INT2EP = 0; // Configure to falling edge
    IEC0bits.INT2IE = 1; // enable interrupt INT1
    
}

void __ISR (_TIMER_2_VECTOR, IPL2SRS) T2_IntHandler (void)//SRS use shadow
{
    TMR2 = 0;
    LATCbits.LATC10 = !LATCbits.LATC10;// Led blinking
    IFS0bits.T2IF = 0; // Clear the T2 interrupt flag
}

void __ISR (_EXTERNAL_1_VECTOR, IPL7SRS) INT1_IntHandler (void)
{
    if(MODE) 
    {
        MODE = 0;
        ON_OFF = 0;//turn off recording
        LATAbits.LATA10 = 0;//turn off recording
        //R_W_ON_OFF = 1;
        T2CONbits.ON = 0; //turn off timer 2
        LATCbits.LATC10 = 0; //turn off led
        
    } else
    {
        MODE = 1;
        ON_OFF = 1;//turn on recording
        LATAbits.LATA10 = 1;
        //R_W_ON_OFF = 0;
        T2CONbits.ON = 1; //turn on timer 2
    }
    
    IFS0bits.INT1IF = 0; // Clear the INT1 interrupt flag
}

void __ISR (_EXTERNAL_2_VECTOR, IPL6SRS) INT2_IntHandler (void)
{
        MODE = 2;
        ON_OFF = 0;//turn off recording
        LATAbits.LATA10 = 0;//turn off recording
        T2CONbits.ON = 0; //turn off timer 2
        LATCbits.LATC10 = 0; //turn off led
        IFS0bits.INT2IF = 0; // Clear the INT2 interrupt flag
}

void __ISR (_UART1_TX_VECTOR, IPL6SRS) U1TX_IntHandler (void)//SRS use shadow
{
    IFS1bits.U1TXIF = 0; // Clear the T2 interrupt flag

}

void __ISR (_UART1_RX_VECTOR, IPL6SRS) U1RX_IntHandler (void)//SRS use shadow
{
    IFS1bits.U1RXIF = 0; // Clear the T2 interrupt flag
    
}

void printGyroAccelConsole(__int16_t *ACCEL_DATA_XYZ, __int16_t *GYRO_DATA_XYZ){
    
    DelayMs(100);

    getAccelData(ACCEL_DATA_XYZ); 
    getGyroData(GYRO_DATA_XYZ);
     
    /*printf("Accelero data :      \n\r");
    printf("X = %.3f m.s-2       \n\r",((((float)(ACCEL_DATA_XYZ[0]))*9.81)/8192));
    printf("Y = %.3f m.s-2       \n\r",((((float)(ACCEL_DATA_XYZ[1]))*9.81)/8192)); 
    printf("Z = %.3f m.s-2       \n\r",((((float)(ACCEL_DATA_XYZ[2]))*9.81)/8192));  
    
    printf("Gyro data :            \n\r");
    //printf("\x1B[s");//save cursor position
    printf("X = %.3f deg.s-1       \n\r",((float)GYRO_DATA_XYZ[0])/(float)131);
    printf("Y = %.3f deg.s-1       \n\r",((float)GYRO_DATA_XYZ[1])/(float)131);  
    printf("Z = %.3f deg.s-1       \n\r",((float)GYRO_DATA_XYZ[2])/(float)131);
    printf("\x1B[1;0H");//set cursor position to origin
          */
    /*printf("Accelero data : \n");
    printf("\x1B[4;0H");//move the cursor back by specifies number of columns
    //printf("\x1B[s");//save cursor position
    printf("X = %d       \n",ACCEL_DATA_XYZ[0]);
    printf("\x1B[5;0H");//move the cursor back by specifies number of columns
    printf("Y = %d       \n",ACCEL_DATA_XYZ[1]);  
    printf("\x1B[6;0H");//move the cursor back by specifies number of columns
    printf("Z = %d       \n",ACCEL_DATA_XYZ[2]);
    //printf("\x1B[u");//restore cursor position saved
    printf("\x1B[4;0H");//set cursor position to origin*/
    
    //printf("\x1B[1;0H");//set cursor position to origin
    printf("X = %.3f        \n\r",((float)GYRO_DATA_XYZ[0])/(float)131);
    //printf("Gyro data :     \n\r");
    //printf("X = %.3f deg.s-1       \n\r",((float)GYRO_DATA_XYZ[0])/(float)131);
    //printf("Y = %.3f deg.s-1       \n\r",((float)GYRO_DATA_XYZ[1])/(float)131);  
    //printf("Z = %.3f deg.s-1       \n\r",((float)GYRO_DATA_XYZ[2])/(float)131);
    //printf("\x1B[2;20H");//set cursor position to origin
    printf("X = %#06x        \n\r",GYRO_DATA_XYZ[0]);
    //printf("\x1B[3;20H");//set cursor position to origin
    //printf("Y = %#06x        ",GYRO_DATA_XYZ[1]); 
    //printf("\x1B[4;20H");//set cursor position to origin
    //printf("Z = %#06x        ",GYRO_DATA_XYZ[2]);
    
         
}

int main(void) {  
    
    fastPerformance();
    initUART1();
    initSPI();
    initMPU9250();
    //initAK8963();
    //calibrateMPU9250();
    
    confExternalInterrupt1();
    confExternalInterrupt2();
    timer2Setup();
    

    __int16_t ACCEL_DATA_XYZ[3] = {0,0,0};
    __int16_t GYRO_DATA_XYZ[3] = {0,0,0};
    __int16_t MAG_DATA_XYZ[3] = {0,0,0};
    __uint32_t endingAddress = 0;
    
    TRISAbits.TRISA10 = 0;   //pin 10 as output
    
    printf("\x1B[2J");//clear console
    
    /*
    //i = EECONbits.ERR;
    //i =(*((int *)(0x00000000 & 0x1fffffff)));
    
    __uint32_t u = 0;
    __uint32_t word[4];
    __uint32_t address = 0x1d010000;
    __uint32_t x = 0; 
    __uint16_t wordDecypher[2];
    //__uint32_t add = u & 0x1fffffff;//convert to physical address
    //printf("0x%x ", add);
    
    getGyroData(GYRO_DATA_XYZ);
    getAccelData(ACCEL_DATA_XYZ);

    word[0] = (__uint32_t)(GYRO_DATA_XYZ[0] << 16) + GYRO_DATA_XYZ[1];
    word[1] = (__uint32_t)(GYRO_DATA_XYZ[2] << 16) + ACCEL_DATA_XYZ[0];
    word[2] = (__uint32_t)(ACCEL_DATA_XYZ[1] << 16) + ACCEL_DATA_XYZ[2];
    word[3] = 0;

        
    //NVMErasePage(0x1d010000);
    writeQuadWordNVM(0x1d010000, word);
    //writeWordNVM(0x1d010000, 0xaaaaaaaa);
    //writeRowWordNVM(0x1d010000, 0);
    for(x=0; x<3; x++){
        memcpy(&u, PA_TO_KVA1(address), 4);
        wordDecypher[0] = u >> 16; //get HSB
        wordDecypher[1] = u; //get HSB
        //printf("dataRead 0x%x ",u);
        printf("Read1 : %d ",wordDecypher[0]);
        printf("Read2 : %d ",wordDecypher[1]);
        address = address + 0x04;
    }
    printf("\nGx : %d",(__uint16_t)GYRO_DATA_XYZ[0]);
    printf("Gy : %d ",(__uint16_t)GYRO_DATA_XYZ[1]);
    printf("Gz : %d ",(__uint16_t)GYRO_DATA_XYZ[2]);
    printf("Ax : %d ",(__uint16_t)ACCEL_DATA_XYZ[0]);
    printf("Ay : %d ",(__uint16_t)ACCEL_DATA_XYZ[1]);
    printf("Az : %d \n",(__uint16_t)ACCEL_DATA_XYZ[2]);
    
   __uint32_t i = 0; 
    __uint32_t x = 0; 
   __uint32_t word[4];
   __int16_t wordDecypher[2];
   __uint32_t *addressWrite = (__uint32_t *)0x1d010000;
   __uint32_t *addressRead = (__uint32_t *)(PA_TO_KVA1(0x1d010000));
            
    for(u = 0 ; u < 100; u++){

        getGyroData(GYRO_DATA_XYZ);
        getAccelData(ACCEL_DATA_XYZ);

        word[0] = (__uint32_t)(GYRO_DATA_XYZ[0] << 16) + GYRO_DATA_XYZ[1];
        word[1] = (__uint32_t)(GYRO_DATA_XYZ[2] << 16) | ACCEL_DATA_XYZ[0];
        word[2] = (__uint32_t)(ACCEL_DATA_XYZ[1] << 16) | ACCEL_DATA_XYZ[2];
        word[3] = 0;

        writeQuadWordNVM(0x1d010000, word);
        
        for(x=0; x<4; x++){
            
            memcpy(&i, PA_TO_KVA1(0x1d010000), 4);

            wordDecypher[0] = (__int16_t)(i >> 16); //get HSB
            wordDecypher[1] = (__int16_t)i; //get HSB

            //printf(" %d ",wordDecypher[0]);
            //printf(" %.3f ",(float)wordDecypher[1]);
            
            printf("Gx : 0x%x ",(__uint16_t)GYRO_DATA_XYZ[0]);
            printf("Gy : 0x%x ",(__uint16_t)GYRO_DATA_XYZ[1]);
            printf("Word0 : 0x%x ",word[0]);
            
            //printf("dataRead1 0x%x ",wordDecypher[0]);
            //printf("dataRead2 0x%x ",wordDecypher[1]);
            //printf("dataRead 0x%x ",i);
            printf("data read 0x%x ",*addressRead);
            printf("Address read 0x%x ",addressRead);

            addressRead = addressRead + 0x04;
        }
        addressWrite = addressWrite + 0x10;
    }*/
    
   
   while(1){
     
        switch (MODE){
            
            case 0 :
                                
                
                printGyroAccelConsole(ACCEL_DATA_XYZ, GYRO_DATA_XYZ);   

                //getMagData(MAG_DATA_XYZ); 

                /*printf("Magnetometer data : \n");
                printf("\x1B[2;0H");//move the cursor back by specifies number of columns
                printf("X = %.3f        ",(float)(MAG_DATA_XYZ[0]));
                printf("\x1B[3;0H");//move the cursor back by specifies number of columns
                printf("Y = %.3f        ",(float)(MAG_DATA_XYZ[1])); 
                printf("\x1B[4;0H");//move the cursor back by specifies number of columns
                printf("Z = %.3f        ",(float)(MAG_DATA_XYZ[2]));
                printf("\x1B[2;0H");//set cursor position to origin */
                
                break;

            case 1 :
                
                printf("\x1B[2J");//clear console
                printf("recording mode ...");
                endingAddress = recordAllSensors(PHYSICAL_STARTING_ADDRESS, 0x1D0100f0, PORTAbits.RA10);
                             
                break;
                
            case 2 :
            
                printf("Dumping Memory ...");
                printf("\x1B[2J");//clear console
                getGyroRecording(PHYSICAL_STARTING_ADDRESS, endingAddress, R_W_ON_OFF);
                
                return 0;
        }
    }

   return 0;
}


