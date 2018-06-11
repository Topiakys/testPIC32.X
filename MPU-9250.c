#include "MPU-9250.h"

char GyroConfig(){
    int GYRO_CONFIG = WriteReadSPI(READ_GYRO_CONFIG);   //read config register
    WriteReadSPI(0x1B00 | (GYRO_CONFIG | GYRO_FS_SEL)); //write config
    GYRO_CONFIG = WriteReadSPI(READ_GYRO_CONFIG);       //read config register
    return GYRO_CONFIG;
}

char AccelConfig(){
    int ACCEL_CONFIG1 = WriteReadSPI(READ_ACCEL_CONFIG1);   //read config register
    int ACCEL_CONFIG2 = WriteReadSPI(READ_ACCEL_CONFIG2);   //read config register
    WriteReadSPI(0x1C00 | (ACCEL_CONFIG1 | ACCEL_FS_SEL));  //write config
    WriteReadSPI(0x1D00 | (ACCEL_CONFIG2 | ACCEL_CONF2));   //write config
    //GYRO_CONFIG = WriteReadSPI(READ_ACCEL_CONFIG);        //read config register
    return ACCEL_CONFIG1;       
}

void initMPU9250(){
    
    ComSPI(PWR_MGMT_1, 0x80, 0);       //reset mpu
    DelayMs(100);
    ComSPI(PWR_MGMT_1, 0x00, 0);       //PWR_MGMT_1 : wake up mpu and select internal 20MHz oscillator
    DelayMs(100);
    ComSPI(PWR_MGMT_2, 0x00, 0);       //PWR_MGMT_2 : enable gyro accelero */
    DelayMs(100);
    GyroConfig();
    AccelConfig();
}

//Can read several register but write only one register each time
void ComAK8963(__uint16_t AdressToBegin, __uint8_t * Data, __uint8_t R_W, __uint8_t NbrOfBytes){
    
    static __uint16_t i = 0;
    __uint8_t i2c_slv0_ctrl = 0x80 | NbrOfBytes;
    
    if(R_W)//if reading request
    {
        ComSPI(I2C_SLV0_ADDR, 0x8C, 0);//Set the I2C slave address of AK8963 and set for read.
        ComSPI(I2C_SLV0_REG, AdressToBegin, 0);
        ComSPI(I2C_SLV0_CTRL, i2c_slv0_ctrl, 0);
        DelayMs(10);
        
        for (i = 0 ; i < NbrOfBytes ; i++){
            Data[i] = ComSPI((EXT_SENS_DATA_00 + i) , 0x00, 1);
        }
    }
    
    else//if writing request
    {
        ComSPI(I2C_SLV0_ADDR, 0x0C, 0);//Set the I2C slave address of AK8963 and set for write.
        ComSPI(I2C_SLV0_REG, AdressToBegin, 0);
        ComSPI(I2C_SLV0_CTRL, i2c_slv0_ctrl, 0);
        DelayMs(10);
        ComSPI(I2C_SLV0_DO, Data[0], 0);
        
    }
    
    ComSPI(I2C_SLV0_CTRL, 0x00, 0);//disable i2c slave
    
    /*ComSPI(EXT_SENS_DATA_00 , 0x00, 0);
    ComSPI((EXT_SENS_DATA_00 + 1) , 0x00, 0);
    ComSPI((EXT_SENS_DATA_00 + 2) , 0x00, 0);
    ComSPI((EXT_SENS_DATA_00 + 3) , 0x00, 0);
    ComSPI((EXT_SENS_DATA_00 + 4) , 0x00, 0);
    ComSPI((EXT_SENS_DATA_00 + 5) , 0x00, 0);*/
    
}

void initAK8963(){

    // I2C Speed 400 kHz
    //and set a stop between reads
    ComSPI(I2C_MST_CTRL, 0x1D, 0);
    DelayMs(10);

    //Attention, registre utilsé plusieurs fois
    //Set I2C_MST_EN & I2C_IF_DIS
    //Enable the I2C Master I/F module;
    //pins ES_DA and ES_SCL are isolated from pins SDA/SDI and SCL/ SCLK.
    //Also Reset I2C Slave module and put the serial interface in SPI mode only
    //Reset I2C Master module
    ComSPI(USER_CTRL, 0x32, 0);
    ComSPI(I2C_SLV0_ADDR, 0x0C, 0);//Set the I2C slave address of AK8963 and set for write.
    ComSPI(I2C_SLV0_REG, AK8963_CNTL2, 0);//begin at CNTL2 address
    ComSPI(I2C_SLV0_DO, 0x01, 0); // Reset AK8963
    //Enable reading data from this slave at the sample rate
    //and storing data at the first available EXT_SENS_DATA register, 
    //which is always EXT_SENS_DATA_00 for I2C slave 0.
    ComSPI(I2C_SLV0_CTRL, 0x81, 0);  
    DelayMs(100);
    
    ComSPI(I2C_SLV0_REG, AK8963_CNTL1, 0);//I2C slave 0 register address from where to begin data transfer
    // Register value to continuous measurement in 16bit
    // 0010 for 8 Hz and 0110 for 100 Hz sample rates.
    ComSPI(I2C_SLV0_DO, 0x16, 0); 
    ComSPI(I2C_SLV0_CTRL, 0x81, 0);  
    DelayMs(100);
    ComSPI(I2C_SLV0_CTRL, 0x00, 0);//disable i2c slave
    

    /*__int8_t id[1] = {2};
    ComAK8963(AK8963_CNTL1,id,1,1);
    printf("%x",id[0] );//read who am I*/

    //ComSPI(AK8963_CNTL1, 0x0F, 0);// Enter Fuse ROM access mode, !!!!POWER DOWN MAG AFTER READING!!!!


}

float getTemp(){
    signed int TEMP_OUT = WriteReadSPI(0xc100);//get HSB first
    TEMP_OUT = (TEMP_OUT << 8) | WriteReadSPI(0xc200); //get LSB + concatenation
    return TEMP_OUT;
}

void getMagData(__int16_t * MAG_DATA){

__uint8_t magData[6] = {0,0,0,0,0,0};

ComAK8963(HXL, magData, 1, 6);
        
/* Get X data */
//Data are stored in little Endian
MAG_DATA[0] = (magData[1] << 8) | magData[0];
    
/* Get Y data */
MAG_DATA[1] = (magData[3] << 8) | magData[2];
    
/* Get Z data */
MAG_DATA[2] = (magData[5] << 8) | magData[4];

}

void getAccelData(__int16_t * ACCEL_DATA){
    
    /* Get X data */
    ACCEL_DATA[0]= ComSPI(ACCEL_XOUT_H, 0x00, 1);//get HSB first
    ACCEL_DATA[0] = (ACCEL_DATA[0] << 8) | (__int16_t)ComSPI(ACCEL_XOUT_L, 0x00, 1); //get LSB + concatenation
    
     /* Get Y data */
    ACCEL_DATA[1]= ComSPI(ACCEL_YOUT_H, 0x00, 1);//get HSB first
    ACCEL_DATA[1] = (ACCEL_DATA[1] << 8) | (__int16_t)ComSPI(ACCEL_YOUT_L, 0x00, 1); //get LSB + concatenation
    
     /* Get Z data */
    ACCEL_DATA[2]= ComSPI(ACCEL_ZOUT_H, 0x00, 1);//get HSB first
    ACCEL_DATA[2] = (ACCEL_DATA[2] << 8) | (__int16_t)ComSPI(ACCEL_ZOUT_L, 0x00, 1); //get LSB + concatenation
    
}

void getGyroData(__int16_t * GYRO_DATA){
    
    __uint16_t X = 0;
    __uint16_t Y = 0;
    __uint16_t Z = 0;
    
    __uint8_t LSB_X = ComSPI(GYRO_XOUT_L, 0x00, 1);
    __int8_t HSB_X = ComSPI(GYRO_XOUT_H, 0x00, 1);
    /* Get X data */
    GYRO_DATA[0] = (__int16_t)(((__int16_t)HSB_X << 8)|LSB_X) ; //doesn't work
    //printf("\x1B[6;20H");//set cursor position to origin
    //printf("X = %#06x        \n\r",GYRO_DATA[0]);
    //printf("X = %#03x        \n\r",LSB_X);
    //printf("X = %#03x        \n\r",HSB_X);
    

    
    //X = (__uint16_t) ComSPI(GYRO_XOUT_H, 0x00, 1);//get HSB first
    //GYRO_DATA[0] = (__int16_t)((X << 8) | (__uint16_t)ComSPI(GYRO_XOUT_L, 0x00, 1)); //get LSB + concatenation
    
    //GYRO_DATA[0]= ComSPI(GYRO_XOUT_H, 0x00, 1);//get HSB first
    //GYRO_DATA[0] = (__int16_t)(GYRO_DATA[0] << 8) + ComSPI(GYRO_XOUT_L, 0x00, 1); //get LSB + concatenation
    
     /* Get Y data */
    Y = (__uint16_t) ComSPI(GYRO_YOUT_H, 0x00, 1);//get HSB first
    GYRO_DATA[1] = (__int16_t)((Y << 8) | ComSPI(GYRO_YOUT_L, 0x00, 1)); //get LSB + concatenation
    
    //__uint8_t LSB_Y = ComSPI(GYRO_YOUT_L, 0x00, 1);
    //__uint16_t HSB_Y = ComSPI(GYRO_YOUT_H, 0x00, 1);
    /* Get Y data */
    //GYRO_DATA[1] = (__int16_t)((HSB_Y << 8)|LSB_Y) ;
    
     /* Get Z data */
    Z = (__uint16_t) ComSPI(GYRO_ZOUT_H, 0x00, 1);//get HSB first
    GYRO_DATA[2] = (__int16_t)((Z << 8) | ComSPI(GYRO_ZOUT_L, 0x00, 1)); //get LSB + concatenation
    
    //GYRO_DATA[2]= ComSPI(GYRO_ZOUT_H, 0x00, 1);//get HSB first
    //GYRO_DATA[2] = (GYRO_DATA[2] << 8) | (__uint16_t)ComSPI(GYRO_ZOUT_L, 0x00, 1); //get LSB + concatenation
    
}

//Read bias into the accel bias registers
//The format are +-8G. The register is initialized with OTP
//factory trim values.
//To calibrate accelero we need to apply the value calculated to these registers
void getAccelBias(__int16_t *ACCEL_BIAS){
    
    __uint8_t data[6] = {0,0,0,0,0,0};
    static int i;
    
    data[0] = ComSPI((XA_OFFSET_H), 0x00, 1);
    data[1] = ComSPI((XA_OFFSET_L), 0x00, 1);
    data[2] = ComSPI((YA_OFFSET_H), 0x00, 1);
    data[3] = ComSPI((YA_OFFSET_L), 0x00, 1);
    data[4] = ComSPI((ZA_OFFSET_H), 0x00, 1);
    data[5] = ComSPI((ZA_OFFSET_L), 0x00, 1);
    
    ACCEL_BIAS[0] = ((__int16_t)data[0]<<8) | data[1];
    ACCEL_BIAS[1] = ((__int16_t)data[2]<<8) | data[3];
    ACCEL_BIAS[2] = ((__int16_t)data[4]<<8) | data[5];
}


// Function which accumulates gyro and accelerometer data after device
// initialization. It calculates the average of the at-rest readings and then
// loads the resulting offsets into accelerometer and gyro bias registers.
void calibrateMPU9250(){
    
    //fifo part
    /*
    __uint16_t fifoCount = 0;
    __uint16_t packetCount = 0;
    __uint16_t z = 0;
    __uint8_t data3[6];
    
    ComSPI(USER_CTRL, 0x04, 0);//reset fifo
    DelayMs(10);
    
    ComSPI(USER_CTRL, 0x40, 0);//set FIFO_EN, enable fifo
    ComSPI(FIFO_EN, 0x70, 0);//enable gyroXYZ fifo in FIFO_EN register
    DelayMs(10);
    ComSPI(USER_CTRL, 0x00, 0);//disable fifo
    
    //ComSPI(USER_CTRL, 0x04, 0);//reset fifo
    //for(i = 0; i<65500; i++);
      
    fifoCount= ComSPI(FIFO_COUNTH, 0x00, 1);//get HSB first
    fifoCount = (fifoCount << 8) | (__int16_t)ComSPI(FIFO_COUNTL, 0x00, 1); //get LSB + concatenation
    printf("fifo count = %d ", fifoCount);
    packetCount = fifoCount/6;
    printf("packet count = %d ", packetCount);
    
    for(z=0; z<packetCount; z++){
        
        data3[z] = ComSPI(FIFO_R_W, 0x00, 1);
        printf("data = %c ", data3[z]);
    
    }
    ComSPI(USER_CTRL, 0x04, 0);//reset fifo
    */
    
    
    //without fifo
    __int16_t accelData[3] = {0,0,0};
    __int16_t gyroData[3] = {0,0,0};
    __int32_t accelBias[3] = {0,0,0};
    __int16_t accelRegBias[3] = {0,0,0};
    __uint8_t maskBit[3] = {0,0,0};
    __int16_t mask = 0x0001; 
    __int32_t gyroBias[3] = {0,0,0};
    static __uint16_t count, i;
    //Current sensitivity divided by the expected sensitivity
    __uint16_t  gyrosensitivity  = 4;// Divide by 4 to get 32.8 LSB per deg/s to conform to expected bias input format.
    __uint16_t  accelsensitivity = 4; // Divide by 8 to get 2048 LSB/g to conform to expected bias input format.
    __uint8_t data[12];//Tab which hold offsets
    
    //Gyro calibration
    for(count = 0; count < 500; count ++)
    {
        getAccelData(accelData); 
        getGyroData(gyroData);
        
        accelBias[0] += (__int32_t) accelData[0];
        accelBias[1] += (__int32_t) accelData[1];
        accelBias[2] += (__int32_t) accelData[2];
        
        gyroBias[0] += (__int32_t) gyroData[0];
        gyroBias[1] += (__int32_t) gyroData[1];
        gyroBias[2] += (__int32_t) gyroData[2];
        
    }
    accelBias[0] = (__int32_t)(((__int64_t)accelBias[0]) / accelsensitivity / count);
    accelBias[1] = (__int32_t)(((__int64_t)accelBias[1]) / accelsensitivity / count);
    accelBias[2] = (__int32_t)(((__int64_t)accelBias[2]) / accelsensitivity / count);
    
    gyroBias[0]  = (__int32_t)(((__int64_t)gyroBias[0]) / gyrosensitivity / count);
    gyroBias[1]  = (__int32_t)(((__int64_t)gyroBias[1]) / gyrosensitivity / count);
    gyroBias[2]  = (__int32_t)(((__int64_t)gyroBias[2]) / gyrosensitivity / count);
    
    //printf("gyro bias x = %f",((float)gyroBias[0])/32.75);
    //printf(", y = %f",((float)gyroBias[1])/32.75);
    //printf(", z = %f",((float)gyroBias[2])/32.75);
    //printf("accel bias x = %f",((float)accelBias[0])*accelsensitivity);
    //printf(", y = %f",((float)accelBias[1])*accelsensitivity);
    //printf(", z = %f",((float)accelBias[2])*accelsensitivity);
    
    // Construct the gyro biases for push to the hardware gyro bias registers,
    // which are reset to zero upon device startup.
    // Biases are additive, so change sign on calculated average gyro biases
    for(i = 0; i<3; i++){
        gyroBias[i] = (-gyroBias[i]);
    }
    data[0] = (gyroBias[0] >> 8) & 0xFF;//HSB
    data[1] = (gyroBias[0]) & 0xFF;//LSB
    data[2] = (gyroBias[1] >> 8) & 0xFF;
    data[3] = (gyroBias[1]) & 0xFF;
    data[4] = (gyroBias[2] >> 8) & 0xFF;
    data[5] = (gyroBias[2]) & 0xFF;
    //Write calculated bias into offset registers
    ComSPI(XG_OFFSET_H, data[0], 0);
    ComSPI(XG_OFFSET_L, data[1], 0);
    ComSPI(YG_OFFSET_H, data[2], 0);
    ComSPI(YG_OFFSET_L, data[3], 0);
    ComSPI(ZG_OFFSET_H, data[4], 0);
    ComSPI(ZG_OFFSET_L, data[5], 0);
    
    /* Get X offset */
    __uint16_t offX= ComSPI(XG_OFFSET_H, 0x00, 1);//get HSB first
    offX = (int16_t)((offX << 8) | (__uint16_t)ComSPI(XG_OFFSET_L, 0x00, 1)); //get LSB + concatenation
    //printf("\x1B[0;15H");
    //printf("x = %#06x                   ",offX);
    //printf("\x1B[6;25H");
    //printf("___gyro offeset x = %.3f",(float)offX/131);
    
    
    // Accel calibration
    
    // Construct the accel biases for push to the hardware accel bias registers,
    // which are reset to the trim calculated value upon device startup.
    getAccelBias(accelRegBias);
    
    //bit 0 of the 3 byte bias is for temp comp
    //find where are these bit and record it
    for(i = 0; i<3; i++){
        if(accelRegBias[i] & mask){
            maskBit[i] = 0x01;
        }
    }
    
    accelRegBias[0] -= accelBias[0];
    accelRegBias[1] -= accelBias[1];
    accelRegBias[2] -= (accelBias[2] - 2048);
    
    data[6] = (accelRegBias[0] >> 8) & 0xFF;//HSB
    data[7] = (accelRegBias[0]) & 0xFF;//LSB
    data[7] = data[7] | maskBit[0];//preserve temperature compensation bit when writing back to accelerometer
    
    data[8] = (accelRegBias[1] >> 8) & 0xFF;
    data[9] = (accelRegBias[1]) & 0xFF;
    data[9] = data[9] | maskBit[1];
    
    data[10] = (accelRegBias[2] >> 8) & 0xFF;
    data[11] = (accelRegBias[2]) & 0xFF;
    data[11] = data[11] | maskBit[2];
    
    //Write calculated bias into offset registers
    ComSPI(XA_OFFSET_H, data[6], 0);
    ComSPI(XA_OFFSET_L, data[7], 0);
    ComSPI(YA_OFFSET_H, data[8], 0);
    ComSPI(YA_OFFSET_L, data[9], 0);
    ComSPI(ZA_OFFSET_H, data[10], 0);
    ComSPI(ZA_OFFSET_L, data[11], 0);
     
}

