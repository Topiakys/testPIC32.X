#include <xc.h>
#include <stdio.h>
#include <string.h>
#include <sys/kmem.h> //to manipulate virtual/physical address

#include "MPU-9250.h"
#include "Flash.h"

#define PHYSICAL_STARTING_ADDRESS 0x1D010000
#define VIRTUAL_STARTING_ADDRESS PA_TO_KVA1(PHYSICAL_STARTING_ADDRESS)
#define PHYSICAL_ENDING_ADDRESS 0x1D0FFF0
#define VIRTUAL_ENDING_ADDRESS PA_TO_KVA1(PHYSICAL_ENDING_ADDRESS)

__uint32_t recordAllSensors(__uint32_t startingAddress, __uint32_t endingAddress, __uint8_t ON_OFF);
void getGyroRecording(__uint32_t startingAddress, __uint32_t endingAddress, __uint8_t ON_OFF);