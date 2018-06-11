#include <xc.h>
#include <stdio.h>
#include "SPI.h"
#include "Delay.h"//Accelerometer register's adress

#define ACCEL_XOUT_H 0x3B
#define ACCEL_XOUT_L 0x3C
#define ACCEL_YOUT_H 0x3D
#define ACCEL_YOUT_L 0x3E
#define ACCEL_ZOUT_H 0x3F
#define ACCEL_ZOUT_L 0x40
//Accel offeset
#define XA_OFFSET_H 0x77
#define XA_OFFSET_L 0x78
#define YA_OFFSET_H 0x7A
#define YA_OFFSET_L 0x7B
#define ZA_OFFSET_H 0x7D
#define ZA_OFFSET_L 0x7E

//Gyrometer register's adress
#define GYRO_XOUT_H 0x43
#define GYRO_XOUT_L 0x44
#define GYRO_YOUT_H 0x45
#define GYRO_YOUT_L 0x46
#define GYRO_ZOUT_H 0x47
#define GYRO_ZOUT_L 0x48
//gyro offset
#define XG_OFFSET_H 0x13
#define XG_OFFSET_L 0x14
#define YG_OFFSET_H 0x15
#define YG_OFFSET_L 0x16
#define ZG_OFFSET_H 0x17
#define ZG_OFFSET_L 0x18

//Magnetometer register's adress
#define AK8963_CNTL1 0x0A
#define AK8963_CNTL2 0x0B
#define HXL 0x03
#define EXT_SENS_DATA_00 0x49

//config magnetometer registers
#define I2C_MST_CTRL 0x24
#define I2C_SLV0_ADDR 0x25
#define I2C_SLV0_REG 0x26
#define I2C_SLV0_CTRL 0x27
#define I2C_SLV0_DO 0x63

#define I2C_SLV1_ADDR 0x28
#define I2C_SLV1_REG 0x29
#define I2C_SLV1_CTRL 0x2A
#define I2C_SLV1_DO 0x2B

//Calibration register's adress
#define FIFO_EN 0x23
#define USER_CTRL 0x6A
#define FIFO_COUNTH 0x72
#define FIFO_COUNTL 0x73
#define FIFO_R_W 0x74

#define READ_GYRO_CONFIG 0x9B00         //adress to read gyro config
#define READ_ACCEL_CONFIG1 0x9C00       //adress to read accel config
#define READ_ACCEL_CONFIG2 0x9D00       //adress to read accel config
#define GYRO_FS_SEL 0x00                //full scale +- 250dps
#define ACCEL_FS_SEL 0x08               //ACCELL_FS_SEL = 1 full scale +- 4g 
#define ACCEL_CONF2 0x00                //ACCEL_FCHOICE = 0 A_DLPF_CFG = 0 => Bandwidth = 1,13 KHz, Delay = 0,75ms, Noise density = 250 ug/rtHz & Rate = 4kHZ 
#define INT_PIN_CFG 0x37                //Use to control the bypass mux
#define PWR_MGMT_1 0x6B
#define PWR_MGMT_2 0x6C
#define SIGNAL_PATH_RESET 0x68
#define I2C_MST_DELAY_CTRL 0x67


void initMPU9250();
char GyroConfig();
void getAccelData(__int16_t * ACCEL_DATA);
void getMagData(__int16_t * MAG_DATA);
float getTemp();
char AccelConfig();
void getGyroData(__int16_t * GYRO_DATA);
void calibrateMPU9250();
void initAK8963();
void testInit();
void getAccelBias(__int16_t *ACCEL_BIAS);
void ComAK8963(__uint16_t AdressToBegin, __uint8_t * Data, __uint8_t R_W, __uint8_t NbrOfBytes);