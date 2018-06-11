void initEEPROM();
void eraseEEPROM();
void readEEPROM(__uint32_t adress, __uint32_t data);
void writeEEPROM(__uint32_t adress, __uint32_t data);

void EEInitialize(void);
void EEWriteExecute(int waitForDone);
void data_EEPROM_write(unsigned int ee_addr, int ee_data) ;
void data_EEPROM_read(unsigned int ee_addr, int *ee_data);