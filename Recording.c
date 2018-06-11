#include "Recording.h"
    
__uint32_t recordAllSensors(__uint32_t startingAddress, __uint32_t endingAddress, __uint8_t ON_OFF){
    
    __uint32_t writeAddress = startingAddress;
    __uint32_t word[4];

    __int16_t ACCEL_DATA_XYZ[3] = {0,0,0};
    __int16_t GYRO_DATA_XYZ[3] = {0,0,0};
    __int16_t MAG_DATA_XYZ[3] = {0,0,0};
    

    while(writeAddress != PHYSICAL_ENDING_ADDRESS)
        
        switch(PORTAbits.RA10)
        {
            case 0 :
                //printf("stoped");
                printf("\x1B[2J");//clear console
                return writeAddress;
                
            case 1 :
                //printf("recording");   
                //NVMErasePage(startingAddress);
                
                getGyroData(GYRO_DATA_XYZ);
                getAccelData(ACCEL_DATA_XYZ);

                word[0] = (__uint32_t)(GYRO_DATA_XYZ[0] << 16) + GYRO_DATA_XYZ[1];
                word[1] = (__uint32_t)(GYRO_DATA_XYZ[2] << 16) + ACCEL_DATA_XYZ[0];
                word[2] = (__uint32_t)(ACCEL_DATA_XYZ[1] << 16) + ACCEL_DATA_XYZ[2];
                word[3] = 0;//future X & Y mag data

                writeQuadWordNVM(writeAddress, word);

                /*
                printf("MGx %d ",GYRO_DATA_XYZ[0]);
                printf("MGy %d ",GYRO_DATA_XYZ[1]);
                printf("MGz %d  ",GYRO_DATA_XYZ[2]);*/

                writeAddress = writeAddress + 0x10;
     
        }
    //printf("finished");
    return writeAddress;
}

void getGyroRecording(__uint32_t startingAddress, __uint32_t endingAddress, __uint8_t ON_OFF){
    
    __uint32_t x,i = 0;
    __uint32_t u = 0;
    __uint16_t wordDecypher[2];
    
    __uint32_t readAddress = startingAddress;
    
    while(readAddress != endingAddress){
        
        switch(ON_OFF)
        {
            case 0 :
                
                return;
                
            case 1 :
    
                for(x=0; x<4; x++){

                    memcpy(&u, PA_TO_KVA1(readAddress), 4);
                    wordDecypher[0] = u >> 16; //get HSB
                    wordDecypher[1] = u; //get HSB

                    switch (x)
                    {
                            case 0 :

                                printf("%d ",(__int16_t)wordDecypher[0]);
                                printf("%d ",(__int16_t)wordDecypher[1]);
                                readAddress = readAddress + 0x04;

                                break;

                            case 1 :

                                printf("%d \n\r",(__int16_t)wordDecypher[0]);
                                readAddress = readAddress + 0x04;

                                break;

                            default :

                                readAddress = readAddress + 0x04;

                                break;
                    }
                }
        }
    }
}

void getAccelRecording(){
    
    __uint32_t x,i = 0;
    __uint32_t u = 0;
    __uint16_t wordDecypher[2];
    
    __uint32_t readAddress = PHYSICAL_STARTING_ADDRESS;
    
    for(i=0; i<10; i++){
    
        for(x=0; x<4; x++){

            memcpy(&u, PA_TO_KVA1(readAddress), 4);
            wordDecypher[0] = u >> 16; //get HSB
            wordDecypher[1] = u; //get HSB

            switch (x)
            {

                    case 1 :

                        printf("Ax %d ",(__int16_t)wordDecypher[1]);
                        readAddress = readAddress + 0x04;

                        break;

                    case 2 :

                        printf("Ay %d ",(__int16_t)wordDecypher[0]);
                        printf("Az %d ",(__int16_t)wordDecypher[1]);
                        readAddress = readAddress + 0x04;

                        break;

                    default :

                        readAddress = readAddress + 0x04;

                        break;
            }
        }
    }
}

void getAllRecording(){
    
    __uint32_t x,i = 0;
    __uint32_t u = 0;
    __uint16_t wordDecypher[2];
    
    __uint32_t readAddress = PHYSICAL_STARTING_ADDRESS;
    
    for(i=0; i<10; i++){
    
        for(x=0; x<4; x++){

            memcpy(&u, PA_TO_KVA1(readAddress), 4);
            wordDecypher[0] = u >> 16; //get HSB
            wordDecypher[1] = u; //get HSB

            switch (x)
            {
                    case 0 :

                        printf("Gx %d ",(__int16_t)wordDecypher[0]);
                        printf("Gy %d ",(__int16_t)wordDecypher[1]);
                        readAddress = readAddress + 0x04;

                        break;

                    case 1 :

                        printf("Gz %d   ",(__int16_t)wordDecypher[0]);
                        //printf("Ax %d ",(__int16_t)wordDecypher[1]);
                        readAddress = readAddress + 0x04;

                        break;

                    /*case 2 :

                        printf("Ay %d ",(__int16_t)wordDecypher[0]);
                        printf("Az %d ",(__int16_t)wordDecypher[1]);
                        readAddress = readAddress + 0x04;

                        break;*/

                    default :

                        readAddress = readAddress + 0x04;

                        break;
            }
        }
    }
}