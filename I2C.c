
//Filename:		I2Clib.c
//Author:		David McCurley
//Version:		1.0
//Date:			2013 Sept 16
//IDE:			MPLabX v1.85
//Compiler:		XC16, v1.11
//Processor:	PIC24FJ256GA110
//Notes:		Currently implements only Master functions (no slave)
//
//The registers I2C1CON and I2C1STAT are specific to this chip family.
//These are blocking functions, meaning they will not return until the operation is complete.
//All functions have error trapping and timeouts, including some timeouts that probably can't happen.
//
//The PIC24FJ256GA110 supports three I2C devices.  This library only implements the first one, but
// it is trivial to implement on other I2C ports.  Just swap out I2C1CON -> I2C2CON, etc.  Also,
// alternate pins are available for I2C2, called "ASCL2,ASDA2".
//
//Usage:
//Init with I2C_SWini(), then I2C_HWini()
//Call I2C1_M_Poll(DevAddr) to clear the bus and verify device is responding
//Then call I2C1_M_Read() and I2C1_M_Write().
//Periodically call I2C1_M_Poll() to verify device is still working.



#include <xc.h>
#include "GenericTypeDefs.h"										//From Microchip\Include\TCPIP_Stack


//-------------------Variables-------------------
WORD I2Cflags;
//-------------------Variables-------------------

//----------------Function Prototypes-------------------
void I2C_HWini(void);
void I2C_ModuleStart(void);
void I2C_SWini(void);
int  I2C1_M_BusReset(void);
void I2C1_M_ClearErrors(void);
int  I2C1_M_Poll(BYTE);
int  I2C1_M_RcvByte(BYTE);
int  I2C1_M_RecoverBus(void);
int  I2C1_M_Read(BYTE, BYTE, int, char *);
int  I2C1_M_ReStart(void);
int  I2C1_M_SendByte(char);
int  I2C1_M_Start(void);
int  I2C1_M_Stop(void);
void I2C1_M_Write(BYTE, BYTE, int, char *);
//----------------Function Prototypes-------------------

void I2C_HWini()
{
	//-----Set pin drive modes-----
	//I2C - drive outputs so we can manually clear lines
	LATDbits.LATD10 = 1;										//Start with bus in idle mode - both lines high
	LATDbits.LATD9 = 1;
	ODCDbits.ODD10 = 1;										//Open drain mode
	ODCDbits.ODD9 = 1;
	TRISDbits.TRISD10 = 0;									//SCL1 output
	TRISDbits.TRISD9 = 0;									//SDA1 output
}

void I2C_ModuleStart()
{
	//Set up I2C for 400kHz operation on I2C port 1 (pins 56,57) on PIC24FJ256GA110
	I2C1CON = 0x1000;										//Set all bits to known state
	I2C1CONbits.I2CEN = 0;									//Disable until everything set up. Pins will be std IO.
	I2C1BRG = 18;	//I2C1BRG = (Fcy/Fscl-FCY/10E6)-1 = (16E6/400E3-16E6/10E6)-1 = 40-1.6-1 = 37.4. Actual Fscl=404kHz.
	I2C1CONbits.DISSLW = 0;									//Enable slew rate control for 400kHz operation
	IFS1bits.MI2C1IF = 0;									//Clear I2C master int flag
	I2C1CONbits.I2CEN = 1;									//Enable I2C

	//For interrupt driven code
	//IEC1bits.MI2C1IE = 1;									//Enable I2C master interrupt
}

void I2C_SWini()
{
	I2Cflags = 0;
	SetI2C1BusDirty;										//I2C bus in unknown state, go ahead and clear it
}

//Reset the I2C bus - called by I2C1_M_RecoverBus()
//This routine will verify that SCL can be set HIGH, then free SDA by clocking until SDA goes HIGH.
//Then the bus will be reset by issuing a START and STOP condition to restart the I2C bus.
//This should clear any devices hung on incomplete I2C transfers.
//
//Note I2C module must not be active or this routine will not work
//Returns:
//0 = OK
//1 = SCL stuck LOW
//2 = SDA stuck LOW (clocking SCL did not release within 10 cycles)
//*******************************************************************************
int I2C1_M_BusReset()
{
	int i;

	//Start with lines high - sets SCL high if not already there
	LATDbits.LATD10 = 1;										//PORTGbits.RG2 = 1 is equivalent
	LATDbits.LATD9 = 1;

	__delay_us(10);											//Need 5uS delay
	if(PORTDbits.RD10 == 0)									//Read if line actually went high
	{
		return 1;											//SCL stuck low - is the pullup resistor loaded?
	}
	//SCL ok, toggle until SDA goes high.
	i=10;
	while(i>0)
	{
		if(PORTDbits.RD9 == 1)								//If SDA is high, then we are done
		{
			break;
		}
		LATDbits.LATD10 = 0;									//SCL low
        __delay_us(10);											//Need 5uS delay
		LATDbits.LATD10 = 1;									//SCL high
        __delay_us(10);											//Need 5uS delay
		i--;
	}
	if((PORTD & 0x0600) != 0x0600)							//We are ok if SCL and SDA high
	{
		return 2;
	}

	LATDbits.LATD9 = 0;										//SDA LOW while SCL HIGH -> START
	__delay_us(10);												//Need 5uS delay
	LATDbits.LATD9 = 1;										//SDA HIGH while SCL HIGH -> STOP
	__delay_us(10);											//Need 5uS delay
	return 0;
}

//Clear any errors that may have occurred
void I2C1_M_ClearErrors()
{
	I2C1CONbits.RCEN = 0;									//Cancel receive request
	I2C1STATbits.IWCOL = 0;									//Clear write-collision flag
	I2C1STATbits.BCL = 0;									//Clear bus-collision flag
}

//Poll an I2C device to see if it is alive
//This should be done periodically, say every 1 second
//Also does error recovery of the I2C bus here, if indicated
//Returns:
//0x0000 Ok
//0x8001 Error
//0x8002 Bus problem
int  I2C1_M_Poll(BYTE DevAddr)
{
	int retval;
	BYTE SlaveAddr;

	SlaveAddr = (DevAddr << 1) | 0;

	if(IsI2C1BusDirty)
	{
		I2C1_M_ClearErrors();
		if(I2C1_M_RecoverBus()==0)
		{//Recovered
			ClrI2C1BusDirty;
		}
		else
		{
			return 0x8002;
		}
	}

	if(I2C1_M_Start() == 0)
	{
		retval = I2C1_M_SendByte((char)SlaveAddr);
		if(I2C1_M_Stop() == 0)								//Even if we have an error sending, try to close I2C
		{
			if(retval == 0)
			{
				return 0;
			}
		}
	}
	//Get here then we had an error
	SetI2C1BusDirty;										//Set error flag
	return 0x8001;
}

//Initiates read of one byte from slave on I2C bus
//Slave must already be addressed, and in read mode
//Waits until completed before returning
//*Caution* Slave can cause a timeout by clock stretching too long
//Returns:
//0x8001 Timeout receiving byte (perhaps CLK line stuck low?)
//0x8002 Timeout while master sending ACK/NACK. Probably impossible for this to happen.
//0x0000-0x00FF
//Error status is indicated by bits set in high byte of returned word
int I2C1_M_RcvByte(BYTE ACKflag)
{
	int t;

	if(ACKflag == I2C_M_NACK)								//Set state in preparation for TX below
	{
		I2C1CONbits.ACKDT = 1;//NACK
	}
	else
	{
		I2C1CONbits.ACKDT = 0;//ACK
	}

	I2C1CONbits.RCEN = 1;									//Start receive
	t=0;//Timeout is processor speed dependent.  @(4*8Mhz=16MIPS) and 8 bits, I expect <=320.
	//We could wait for RCEN to be cleared, but are really interested in incoming byte, so look for I2C1STAT.RBF
	while(!I2C1STATbits.RBF)								//HW cleared when receive complete
	{
		t++;
		if(t>8000)
		{
			return 0x8001;
		}
	}//Tested: t=30
	//I2C1STATbits.RBF will likely be set

	//As the master we must ACK or NACK every byte, so slave knows if it will send another byte.
	//We have set the bit above, just need to send it
	I2C1CONbits.ACKEN = 1;									//Send ACK bit now
	t=0;//Timeout is processor speed dependent.  @(4*8Mhz=16MIPS), I expect <=40.
	while(I2C1CONbits.ACKEN)								//HW cleared when complete
	{
		t++;
		if(t>1000)
		{
			return 0x8002;
		}
	}//Tested: t=4
	if(I2C1STATbits.I2COV)									//If an overflow occurred, it means we received a new byte before reading last one
	{
		I2C1STATbits.I2COV = 0;								//Ignore the error, but watch for this breakpoint!
	}

	return I2C1RCV;											//Reading this register clears RBF
}

//Attempt to recover after I2C error
//Returns:
//0x0000 Success
//0xFFFF Failure
int I2C1_M_RecoverBus()
{
	int status;
//	//Level 1: reset the I2C hardware on our side
//	I2C1CONbits.I2CEN = 0;
//	Nop();
//	I2C1CONbits.I2CEN = 1;

	//Level 2: reset devices on I2C network
	//Disable I2C so we can toggle pins
	I2C1CONbits.I2CEN = 0;
	status = I2C1_M_BusReset();
	if(status>0)
	{//Fatal I2C error, nothing we can do about it
		return 0xFFFF;
	}
	//That worked, bring I2C back online
	I2C1CONbits.I2CEN = 1;

	return 0;
}

//High level function.  Reads data from target into buffer
//Returns:
//0x0000 Success
//0x8001 Error
int  I2C1_M_Read(BYTE DevAddr, BYTE SubAddr, int ByteCnt, char *buffer)
{
	BYTE SlaveAddr;
	int retval;
	int i;

	if(IsI2C1BusDirty)//Ignore requests until Poll cmd is called to fix err.
        return 0x8001;

	SlaveAddr = (DevAddr << 1) | 0;							//Device Address + Write bit

	if(I2C1_M_Start() != 0)									//Start
	{//Failed to open bus
		SetI2C1BusDirty;
		return 0x8001;
	}

	if( I2C1_M_SendByte((char)SlaveAddr) != 0)				//Slave Addr
	{
		I2C1_M_Stop();
		SetI2C1BusDirty;
		return 0x8001;
	}

	if( I2C1_M_SendByte((char)SubAddr) != 0)				//Sub Addr
	{
		I2C1_M_Stop();
		SetI2C1BusDirty;
		return 0x8001;
	}

	if( I2C1_M_ReStart() != 0)								//Repeated start - switch to read mode
	{
		I2C1_M_Stop();
		SetI2C1BusDirty;
		return 0x8001;
	}

	SlaveAddr = (DevAddr << 1) | 0x01;						//Device Address + Read bit

	if( I2C1_M_SendByte((char)SlaveAddr) != 0)				//Slave Addr
	{
		I2C1_M_Stop();
		SetI2C1BusDirty;
		return 0x8001;
	}
	for(i=0;i<ByteCnt;i++)									//Data
	{
		if(i==(ByteCnt-1) )
		{
			retval = I2C1_M_RcvByte(I2C_M_NACK);			//NACK on last byte so slave knows this is it
		}
		else
		{
			retval = I2C1_M_RcvByte(I2C_M_ACK);
		}
		if(retval >= 0)
		{
			buffer[i] = retval;
		}
		else
		{//Error while reading byte.  Close connection and set error flag.
			I2C1_M_Stop();
			SetI2C1BusDirty;
			return 0x8001;
		}
	}

	if(I2C1_M_Stop() != 0)
	{//Failed to close bus
		SetI2C1BusDirty;
		return 0x8001;
	}
	return 0;												//Success
}

//Initiates repeated start sequence on I2C bus
//Waits until completed before returning
//Returns:
//0 Success
//0x8001 Timeout (unlikely to happen unless hardware issue)
//0x8002 Bus collision detected
int I2C1_M_ReStart()
{
	int t;

	I2C1CONbits.RSEN = 1;									//Initiate restart condition
	Nop();
	if(I2C1STATbits.BCL)
	{
		return 0x8002;										//Will need to clear BCL!
	}

	t=0;//Timeout is processor speed dependent.  @(4*8Mhz=32Mhz;16MIPS), I expect <=40.
	while(I2C1CONbits.RSEN)									//HW cleared when complete
	{
		t++;
		if(t>1000)
		{
			return 0x8001;
		}
	}//Tested: t=5
	return 0;
}

//Sends a byte to a slave
//Waits until completed before returning
//Returns:
//1 NACK
//0 ACK
//0x8001 Transmit buffer full (if you get this you have a programming error!)
//0x8002 Timeout (CLK stuck low or slave stretched clock longer than our timeout)
//0x8003 Bus collision detected (DAT stuck low, detected after TX starts. Will likely have to reset hung I2C device)
int I2C1_M_SendByte(char cData)
{
	int t;

	if(I2C1STATbits.TBF)									//Is there already a byte waiting to send?
	{
		return 0x8001;
	}
	I2C1TRN = cData;										//Send byte
	//Transmission takes several clock cycles to complete.  As a result we won't see BCL error for a while.
	t=0;//Timeout is processor speed dependent.  @(4*8Mhz=32Mhz;16MIPS) and 8 bits, I expect <=320.
	while(I2C1STATbits.TRSTAT)								//HW cleared when TX complete
	{
		t++;
		if(t>8000)
		{//This is bad because TRSTAT will still be set
			return 0x8002;
		}
	}//Testing: t=31

	if(I2C1STATbits.BCL)
	{
		I2C1STATbits.BCL = 0;								//Clear error to regain control of I2C
		return 0x8003;
	}

	//Done, now how did slave respond?
	if(I2C1STATbits.ACKSTAT)								//1=NACK
		return 1;											//  NACK
	else
		return 0;											//  ACK
}

//Initiates start sequence on I2C bus
//Waits until completed before returning
//Returns:
//0 Success
//0x8001 Timeout (should not happen)
//0x8002 Bus collision - CLK or DAT line stuck low
//0x8003 Write collision
int I2C1_M_Start()
{
	int t;

	I2C1CONbits.SEN = 1;									//Initiate Start condition
	Nop();
	if(I2C1STATbits.BCL)
	{
		I2C1CONbits.SEN = 0;								//Cancel request (will still be set if we had previous BCL)
		I2C1STATbits.BCL = 0;								//Clear error to regain control of I2C
		return 0x8002;
	}
	if(I2C1STATbits.IWCOL)
	{//Not sure how this happens but it occurred once, so trap here
		I2C1CONbits.SEN = 0;								//Clear just in case set
		I2C1STATbits.IWCOL = 0;								//Clear error
		return 0x8003;
	}

	t=0;//Timeout is processor speed dependent.  @(4*8Mhz=32Mhz;16MIPS), I expect <=40.
	while(I2C1CONbits.SEN)									//HW cleared when complete
	{
		t++;
		if(t>1000)
		{
			return 0x8001;
		}
	}//Tested: t=3.  I2C1STATbits.S will be set indicating start bit detected.

	//If a second start request is issued after first one, the I2C module will instead:
	//generate a stop request, clear SEN, and flag BCL.  Test for BCL here.
	if(I2C1STATbits.BCL)
	{
		I2C1STATbits.BCL = 0;								//Clear error to regain control of I2C
		return 0x8002;
	}

	return 0;
}

//Initiates stop sequence on I2C bus
//Waits until completed before returning
//Returns:
//0 Success
//0x8001 Timeout (unlikely to happen unless hardware issue)
//0x8002 Bus collision detected
int I2C1_M_Stop()
{
	int t;

	I2C1CONbits.PEN = 1;									//Initiate stop condition
	Nop();
	if(I2C1STATbits.BCL)
	{
		return 0x8002;											//Will need to clear BCL!
	}

	t=0;//Timeout is processor speed dependent.  @(4*8Mhz=16MIPS), I expect <=40.
	while(I2C1CONbits.PEN)									//HW cleared when complete
	{
		t++;
		if(t>1000)
		{
			return 0x8001;
		}
	}//Tested: t=5
	return 0;
}

//High level function.  Writes buffered data to target address.
//Returns: Nothing.  Errors are handled internally.
void I2C1_M_Write(BYTE DevAddr, BYTE SubAddr, int ByteCnt, char *buffer)
{
	int i;
	BYTE SlaveAddr;

	if(IsI2C1BusDirty)										//Ignore requests until Poll cmd is called to fix err.
		return;

	SlaveAddr = (DevAddr << 1) | 0;							//Device Address + Write bit

	if(I2C1_M_Start() != 0)									//Start
	{//Failed to open bus
		SetI2C1BusDirty;
		return;
	}

	if( I2C1_M_SendByte((char)SlaveAddr) != 0)				//Slave Addr
	{
		I2C1_M_Stop();
		SetI2C1BusDirty;
		return;
	}

	if( I2C1_M_SendByte((char)SubAddr) != 0)				//Sub Addr
	{
		I2C1_M_Stop();
		SetI2C1BusDirty;
		return;
	}

	for(i=0;i<ByteCnt;i++)									//Data
	{
		if( I2C1_M_SendByte(buffer[i]) != 0)
		{//Error while writing byte.  Close connection and set error flag.
			I2C1_M_Stop();
			SetI2C1BusDirty;
			return;
		}
	}

	if(I2C1_M_Stop() != 0)
	{//Failed to close bus
		SetI2C1BusDirty;
		return;
	}
	return;
}
