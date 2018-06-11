#include <xc.h>
#include "Delay.h"

/*Attention au bit WR, à verifier*/
void initEEPROM(){
    
    /* Unlock the EECON register for write/erase operations */
     __builtin_disable_interrupts();
    EEKEYbits.EEKEY = 0xEDB7;
    EEKEYbits.EEKEY = 0x1248;
    __builtin_enable_interrupts(); 
    EECONbits.ON = 1;// enable data EEPROM
    
    
}

void eraseEEPROM(){
    
    /* Unlock the EECON register for write/erase operations */
     __builtin_disable_interrupts();
    EEKEYbits.EEKEY = 0xEDB7;
    EEKEYbits.EEKEY = 0x1248;
    __builtin_enable_interrupts(); 
    
   if (EECONbits.RDY==1) {          // If Data EEPROM to be ready
        if (EECONbits.RW==0) {      // If no operation underway
                                    // Execute erase command
            EECONbits.CMD = 3;      // Load CMD<2:0> with erase command 
            EECONbits.WREN = 1;     // Access for write or erase
            EEKEY = 0xED87;         // Write unlock sequence
            EEKEY = 0x1248;
            EECONbits.RW = 1;       // Start the erase cycle 

            while (EECONbits.RW==1);//Wait for erase cycle to complete
        }
    }
}

void writeEEPROM(__uint32_t adress, __uint32_t data){
    
    /* Unlock the EECON register for write/erase operations */
    __builtin_disable_interrupts();
    EEKEYbits.EEKEY = 0xEDB7;
    EEKEYbits.EEKEY = 0x1248;
    __builtin_enable_interrupts(); 
    EECONbits.WREN = 1;// Enables program or erase operations
    EECONbits.CMD = 1;// Word Write command (WREN bit must be set)
    EEADDR = adress;
    EEDATA = data;
    
    while(!EECONbits.RDY);
    
    // The Data EEPROM Unlock sequence must be executed prior to attempting a 
    // write or erase cycle
    __builtin_disable_interrupts();
    EEKEY = 0xED87; // Write unlock sequence 
    EEKEY = 0x1248;
    __builtin_enable_interrupts(); 
    EECONbits.RW = 1;// Start write command

}

void readEEPROM(__uint32_t adress, __uint32_t data){
    
    /* Unlock the EECON register for write/erase operations */
    __builtin_disable_interrupts();
    EEKEYbits.EEKEY = 0xEDB7;
    EEKEYbits.EEKEY = 0x1248;
    __builtin_enable_interrupts(); 
    EECONbits.WREN = 0;// Disables program or erase of memory elements, and enables read operations
    EECONbits.CMD = 0;// Word Read command (WREN bit must be clear)
    EEADDR = data & 0xFFC;   // Load address on a 32-bit boundary
    
    while(!EECONbits.RDY);
    
    //The unlock sequence is not necessary for a read command.
    EECONbits.RW = 1;// Start read command
    while(EECONbits.RW);// Waiting operation is completed
    data = EEDATA;

}

void EEWriteExecute(int waitForDone) {
    
    /* Disable interrupts prior to unlock sequence */
    __builtin_disable_interrupts();
    EEKEY = 0xEDB7;             // Unlock the EEPROM to enable writing
    EEKEY = 0x1248;             // Execute the write
    
    EECONbits.RW = 1;
    
    /* Re-enable interrupts */
    __builtin_enable_interrupts();
    
    if (waitForDone)            // Wait for the write to finish if
        while (EECONbits.RW);       // desired
} 

/*  Before accessing the EEPROM at full speed, it is necessary to program configuration values into
 *  the Data EEPROM controller after enabling it. This is done through the Configuration 
 *  Write command (CMD<2:0> bits (EECON<2:0>) = 100). The configuration values to be 
 *  written to the Data EEPROM Controller are stored in the DEVEE1 through DEVEE3 registers
*/
void EEInitialize(void) // Basic EEPROM enable and initialization
{
    EECONbits.ON = 1;// Turn on EEPROM
    
    while (EECONbits.RDY == 0);// Wait until EEPROM is ready (~125 us)
    
    EECONbits.WREN = 1;// Enable writing to the EEPROM
    
    EECONbits.CMD = 0b100;// Set the command to Configuration Write
    
    EEADDR = 0x00;// Addr 0x00 = DEVEE0;
    EEDATA = DEVEE0;
    EEWriteExecute(1); // Execute write and wait for finish
    
    EEADDR = 0x10;// Addr 0x04 = DEVEE1;
    EEDATA = DEVEE1;
    EEWriteExecute(1); // Execute write and wait for finish
    
    EEADDR = 0x20;// Addr 0x08 = DEVEE2;
    EEDATA = DEVEE2;
    EEWriteExecute(1); // Execute write and wait for finish
    
    EEADDR = 0x30;// Addr 0x08 = DEVEE3;
    EEDATA = DEVEE3;
    EEWriteExecute(1); // Execute write and wait for finish
    
    EECONbits.WREN = 0; // Turn off writes. }
}

void data_EEPROM_write(unsigned int ee_addr, __uint32_t ee_data) {
    
    if (EECONbits.RDY==1) {             // If Data EEPROM to be ready
        
        if (EECONbits.RW==0) {          // If no operation underway
            
                                        // Execute Write Command
            EEADDR = ee_addr & 0xFFC;   // Load address on a 32-bit boundary 
            EECONbits.CMD = 1;          // Load CMD<2:0> with write command 
            EECONbits.WREN = 1;         // Access for write
            EEDATA = ee_data;
            
            EEKEY = 0xED87;             // Write unlock sequence
            EEKEY = 0x1248;
            
            EECONbits.RW = 1;           // Start the write cycle

            while (EECONbits.RW==1);    //Wait for write cycle to complete
        }
    }
}

void data_EEPROM_read(unsigned int ee_addr, int *ee_data) {
    
    if (EECONbits.RDY==1) {             // If Data EEPROM to be ready
        
        if (EECONbits.RW==0) {          // If no operation underway
                                        // Execute Read Command
            EEADDR = ee_addr & 0xFFC;   // Set address on 32-bit boundary
            EECONbits.CMD = 0;          // Load CMD<2:0> with
                                        // Data EEPROM read command
            EECONbits.WREN = 0;         // Access for read
            EECONbits.RW = 1;           // Start the operation
            while (EECONbits.RW==1);    // Wait until read is complete
            *ee_data = EEDATA;          // Read the data
        }
    }
}



 




