#include <xc.h>

#define BAUDRATE 230400
#define CLK_FREQUENCY 64000000

void initUART1(){

    TRISBbits.TRISB3 = 0; //B3 as output 18 ->PIN23
    TRISBbits.TRISB2 = 1; //B2 as input 17 ->PIN22
    RPB3R = 0; //unMap RPB3
    U1RXR = 0; //unMap U1RX 
    
    U1MODE = 0; //erase register
    U1MODEbits.CLKSEL = 1; //BRG clock is SYSCLK (off in Sleep mode)
    U1MODEbits.PDSEL = 0; //8-bit data, no parity
    U1MODEbits.STSEL = 0; //1 Stop bit
    U1MODEbits.WAKE = 1;  // Wake-up enabled
    U1BRG = ((CLK_FREQUENCY/(16 * BAUDRATE))-1);
    U1MODEbits.ON = 1; //enable UART1
    U1STAbits.URXEN = 1; //UARTx receiver is enabled. UxRX pin is controlled by UARTx (if ON bit (UxMODE<15>) = 1)
    U1STAbits.UTXEN = 1; //UARTx transmitter is enabled. UxTX pin is controlled by UARTx (if ON bit (UxMODE<15>) = 1)
   
    RPB3Rbits.RPB3R = 1; //Map U1TX on RPB3
    U1RXR = 4; //Map U1RX RPB2
    /*
    //configure UART1 Rx interrupt
    IEC1bits.U1EIE = 0; //disable interrupt U1
    IFS1bits.U1RXIF = 0; // Clear the Rx interrupt flag
    IPC9bits.U1RXIP = 6; // Set Rx interrupt priority to 7 (highest) bit26:28)
    IPC9bits.U1RXIS = 3; // subpriority to 3 (max) bit 24:25
    IEC1bits.U1EIE = 1; //enable interrupt U1
    IEC1bits.U1RXIE = 1; //enable interrupt RX
    PRISSbits.PRI6SS = 1; //Interrupt with a priority level of 7 uses Shadow Set 1
    
    //configure UART1 Tx interrupt
    IEC1bits.U1EIE = 0; //disable interrupt U1
    IFS1bits.U1TXIF = 0; // Clear the T2 interrupt flag
    IPC10bits.U1TXIP = 6; // Set OC1 interrupt priority to 7 (highest) bit26:28)
    IPC10bits.U1TXIS = 3; // subpriority to 3 (max) bit 24:25
    IEC1bits.U1EIE = 1; //enable interrupt U1
    IEC1bits.U1TXIE = 1; //enable interrupt TX
    PRISSbits.PRI6SS = 1; //Interrupt with a priority level of 7 uses Shadow Set 1
    */
   
}

int i =0;

signed int modulo10(signed int num)
{
    while (num >= 10)
    {
        num -= 10; 
    }
    return num; 
}

void _mon_putc (char c)
{
   while (U1STAbits.UTXBF);
   U1TXREG = c;
}
void
print1(char *string){
    
    U1STAbits.UTXEN = 1;                // Make sure transmitter is enabled
    
    while(*string){
        while(U1STAbits.UTXBF);// Wait while buffer is full
        U1TXREG = *string;
        string++;
       }
}

void printNbr(signed int number){
    
    char i = 0;
    char u,r = 0;
    char z[10];
    
    while(number>0){
        r = modulo10(number);
        z[i] = (signed char)48 + r;
        number = number/10;
        i++;
    }
    
    U1STAbits.UTXEN = 1;                // Make sure transmitter is enabled
    
    for(u = 0; u < i ; u++){
        while(U1STAbits.UTXBF);// Wait while buffer is full
        U1TXREG = z[i-1-u];
       }
    
}

char ReadChar(void)
{
    while(!U1STAbits.URXDA); 
    return U1RXREG;                     // Return received character
}


void ReadString(char *string, int length)
{  
    int count = length;
     
    do
    {
        *string = ReadChar();               // Read in character
        //SendChar(*string);                  // Echo character
         
        if(*string == 0x7F && count>length) // Backspace conditional
        {
            length++;
            string--;
            continue;
        }
         
        if(*string == '\r')                 // End reading if enter is pressed
            break;
         
        string++;
        length--;
         
    }while(length>1);
     
    *string = '\0';                         // Add null terminator
}

