#include <xc.h>
#include <stdio.h>
#include  <sys/kmem.h> //to manipulate virtual/physical address

void NVMInitiateOperation(void) {
    
    int int_status; // storage for current Interrupt Enable state 
    int dma_susp; // storage for current DMA state
    
    // Disable Interrupts
    __builtin_disable_interrupts();
    
    // Disable DMA 
    if(!(dma_susp=DMACONbits.SUSPEND)) {
    
        DMACONSET=_DMACON_SUSPEND_MASK;         // suspend
        while((DMACONbits.DMABUSY));            // wait to be actually suspended
    }
    
    NVMKEY = 0x0;
    NVMKEY = 0xAA996655;
    NVMKEY = 0x556699AA; 
    NVMCONSET = 1 << 15;   // must be an atomic instruction
    //NVMCONbits.WR = 1; //Initiate a Flash operation
    
    // Restore DMA
    if(!dma_susp)
    {
        DMACONCLR=_DMACON_SUSPEND_MASK;         // resume DMA activity
    }
    
    __builtin_enable_interrupts();
}

void writeWordNVM(__uint32_t address, __uint32_t data){
      
    // Set up Address and Data Registers
    NVMADDR = address; // physical address 
    NVMDATA0 = data; // value
    
    // set the operation, assumes WREN = 0
    NVMCONbits.NVMOP = 0x1; // NVMOP for Word programming
    
    // Enable Flash for write operation and set the NVMOP 
    NVMCONbits.WREN = 1;

    // Start programming
    NVMInitiateOperation(); // see Example 52-1

    // Wait for WR bit to clear 
    while(NVMCONbits.WR);

    // Disable future Flash Write/Erase operations 
    NVMCONbits.WREN = 0;

    // Check Error Status 
    if(NVMCON & 0x3000) {
        printf("error");
        printf(" LVDERR %d", NVMCONbits.LVDERR);
        printf(" WRERR %d", NVMCONbits.WRERR);
    }

}

void writeQuadWordNVM(__uint32_t address, __uint32_t * data){
      

    // Set up Address and Data Registers
    NVMADDR = address; // physical address
    NVMDATA0 = data[0]; 
    NVMDATA1 = data[1]; 
    NVMDATA2 = data[2]; 
    NVMDATA3 = data[3];

    // set the operation, assumes WREN = 0
    NVMCONbits.NVMOP = 0x2; // NVMOP for Quad Word programming

    // Enable Flash for write operation and set the NVMOP 
    NVMCONbits.WREN = 1;

    // Start programming
    NVMInitiateOperation(); // see Example 52-1

    // Wait for WR bit to clear 
    while(NVMCONbits.WR);

    // Disable future Flash Write/Erase operations 
    NVMCONbits.WREN = 0;

    // Check Error Status 
    if(NVMCON & 0x3000) {
        printf("error");
        printf(" LVDERR %d", NVMCONbits.LVDERR);
        printf(" WRERR %d", NVMCONbits.WRERR);
    }

}

void writeRowWordNVM(__uint32_t address, __uint32_t * data){
     
    unsigned int rowbuff[128]; // example is for a 512 word row size.
    int x = 0; // loop counter
    
    // put some data in the source buffer
    for (x = 0; x < 128; x++)
        rowbuff[x] = x;
    
    // set destination row address
    NVMADDR = address; // row physical address
    
    // set source address. Must be converted to a physical address.
    NVMSRCADDR = (unsigned int) KVA_TO_PA(rowbuff);
    
    // define Flash operation 
    NVMCONbits.NVMOP = 0x3;// NVMOP for Row programming
    
    // Enable Flash Write 
    NVMCONbits.WREN = 1;
    
    // begin programming 
    NVMInitiateOperation();
    
    // Wait for WR bit to clear 
    while(NVMCONbits.WR);
    
    // Disable future Flash Write/Erase operations 
    NVMCONbits.WREN = 0;

    // Check Error Status 
    if(NVMCON & 0x3000) {
        printf("error");
        printf(" LVDERR %d", NVMCONbits.LVDERR);
        printf(" WRERR %d", NVMCONbits.WRERR);
    }

}

void NVMErasePage(__uint32_t address){

    // set destination page address
    NVMADDR = address;   // page physical address
    
    //WRERR must be cleared before commencing Flash program or erase operations
    //printf(" WRERR %d", NVMCONbits.WRERR);
    
    // define Flash operation 
    NVMCONbits.NVMOP = 0x4; // NVMOP for Page Erase
    
    // Enable Flash Write
    NVMCONbits.WREN = 1;
    
    // commence programming 
    NVMInitiateOperation();
    
    // Wait for WR bit to clear 
    while(NVMCONbits.WR);
    
    // Disable future Flash Write/Erase operations 
    NVMCONbits.WREN = 0;
    
    // Check Error Status
    if(NVMCON & 0x3000)
    {
        printf("Erase error");
        printf(" LVDERR %d", NVMCONbits.LVDERR);
        printf(" WRERR %d", NVMCONbits.WRERR);
    }}