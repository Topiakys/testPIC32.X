void NVMInitiateOperation(void);
void writeNVM(__uint32_t address, __uint32_t data);
void NVMErasePage(__uint32_t address);
void writeQuadWordNVM(__uint32_t address, __uint32_t * data);
void writeRowWordNVM(__uint32_t address, __uint32_t * data);