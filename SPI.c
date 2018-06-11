#include <xc.h>

#define BAUDRATE 230400
#define FPBCLK 64000000

#define CS1 LATCbits.LATC6
#define CS2 LATBbits.LATB9

void initSPI(void)
{
    TRISCbits.TRISC6 = 0;       //CS1 as output
    TRISBbits.TRISB9 = 0;       //CS2 as output
    TRISBbits.TRISB7 = 0;       //SCK1 as output
    TRISCbits.TRISC8 = 0;       //SDO1 as output
    TRISCbits.TRISC7 = 1;       //SDI1 as input
    RPC8R = 3;
    SDI1R = 5;
    CS1 = 1;
    CS2 = 1;                     // Set CS high (idle state)
     
    IEC1bits.SPI1EIE = 0;       // SPI interrupts disabled
    IEC1bits.SPI1RXIE = 0;
    IEC1bits.SPI1TXIE = 0;
     
    SPI1CONbits.ON = 0;         // Turn off SPI module
     
    SPI1BUF = 0;                // Clear the receive buffer
     
    SPI1BRG = (FPBCLK / (2 * BAUDRATE)) - 1;                // FSCK = 64MHz
     
    SPI1STATbits.SPIROV = 0;    // Clear overflow flag
     
     
    /* SPI1CON settings */
    SPI1CONbits.FRMEN = 0;      // Framed SPI support is disabled
    SPI1CONbits.SIDL = 0;       // Continue operation in IDLE mode
    SPI1CONbits.DISSDO = 0;     // SDO1 pin is controlled by the module
    SPI1CONbits.MODE16 = 1;     // 16 bit mode
    SPI1CONbits.MODE32 = 0;
    SPI1CONbits.CKP = 1;        // Idle state for clock is high, active state is low
    SPI1CONbits.CKE = 0;        // Output data changes on transition from idle to active
    SPI1CONbits.SSEN = 0;       // Not in slave mode
    SPI1CONbits.MSTEN = 1;      // Master mode
    SPI1CONbits.SMP = 1;        // Input data sampled at the end of data output time
     
    SPI1CONbits.ON = 1;         // Turn module on
}

char WriteReadSPI(short i)
{
    CS2 = 0;                         // Set the chip select low
    SPI1BUF = i;                    // Write to buffer for transmission
    while (!SPI1STATbits.SPIRBF);   // Wait for transfer to be completed
    CS2 = 1;                         // Set the chip select back high
    return SPI1BUF;                 // Return the received value
}

//read or write register, then return de register
//Read = 1 & Write = 0
__uint8_t ComSPI(__uint16_t Adress, __uint8_t Data, __uint8_t R_W)
{
    __uint16_t BUF = 0;
    switch (R_W)
    {
        case 1:
            Adress <<= 8;
            BUF = Adress | 0x8000; //Add read bit
            break;
            
        case 0:
            Adress <<= 8;
            BUF = Adress | Data;
            break;
    }
    CS2 = 0;                         // Set the chip select low
    SPI1BUF = BUF;                    // Write to buffer for transmission
    while (!SPI1STATbits.SPIRBF);   // Wait for transfer to be completed
    CS2 = 1;                         // Set the chip select back high
    return SPI1BUF;                 // Return the received value
}