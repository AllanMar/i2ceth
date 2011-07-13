#include "BTnic_Comm.h"
#include <i2c.h>
#include "TCPIP Stack/Tick.h"

volatile char BTCommBuffer[BTCOMM_BUFFER_SIZE];
volatile unsigned int BTCommLen, BTCommCur;
volatile BYTE BTCommState;
volatile unsigned long BTCommTimer;

void BTCommInit(void)
{
	//I2C Pins as Inputs
	I2C_SCL_TRIS = 1;
	I2C_SDA_TRIS = 1;
	
	//Reset BTCommState
	BTCommSetState(BT_COMMSTATE_IDLE);

	//Enter Slave Mode
	PIE1bits.SSP1IE = 0;           //Turn off I2C/SPI interrupt    
	PIR1bits.SSP1IF = 0;           //Clear any pending interrupt    

	SSP1STAT = 0x80;   //Disable SMBus & Slew Rate Control
 	SSP1CON1 = 0x26;  //I2C 7-Bit Slave
	SSP1ADD = I2C_BTNICSLAVE_ADDR;
	PIE1bits.SSP1IE = 1; 

	//Enable I2C interrupts    
	INTCONbits.PEIE = 1;          //Turn on peripheral interrupts    
	INTCONbits.GIE = 1;           //Turn on global interrupts
}

int BTCommTX(char* reqMsg)
{
	char c;
	if (BTCommState == BT_COMMSTATE_IDLE) BTCommSetState(BT_COMMSTATE_TX);
	
	PIE1bits.SSP1IE = 0;           //Turn off I2C/SPI interrupt    
	PIR1bits.SSP1IF = 0;           //Clear any pending interrupt    

	SSP1CON1 = 0x00;
 	SSP1CON1 = 0x28;  //I2C Master
	SSP1ADD = I2C_MASTER_BAUDRATE; //Set I2C Speed
	SSP1CON2 = 0x00;

	PIR2bits.BCL1IF = 0; //Clear any previous collision flag

	IdleI2C1();
	StartI2C1();
    if (PIR2bits.BCL1IF) return ( -1 );	// test for bus collision

    if (WriteI2C1(I2C_BTSLAVE_ADDR)) 
	{
		StopI2C1();
		return ( -3 );	// set error for write collision
	}

	if (SSP1CON2bits.ACKSTAT) 
	{                 
		StopI2C1();                  
		return ( -2 );      // return with Not Ack error condition                      
	}

	while (1) {
	    while(*reqMsg != '\0') 
		{
			c = *reqMsg++;
			if (c == '+') c = ' ';
			if (WriteI2C1(c))
			{
				StopI2C1();
				return ( -3 );	// set error for write collision
			}
			if (SSP1CON2bits.ACKSTAT) 
			{                 
				StopI2C1();                  
				return ( -2 );      // return with Not Ack error condition                      
			}
	    }
		if (*(reqMsg + 1) == '\0') break;
		*reqMsg = '\t';
	}

	if (WriteI2C1('\r'))
	{
		StopI2C1();
		return ( -3 );	// set error for write collision
	}
	if (SSP1CON2bits.ACKSTAT) 
	{                 
		StopI2C1();                  
		return ( -2 );      // return with Not Ack error condition                      
	}

	StopI2C1();
	while ( SSP1CON2bits.PEN );      // wait until stop condition is over
    if (PIR2bits.BCL1IF) return ( -1 );	// test for bus collision

	BTCommSetState(BT_COMMSTATE_WAIT);
	
	//Re-Enter Slave Mode
	SSP1CON1 = 0x00;
 	SSP1CON1 = 0x26;  //I2C 7-Bit Slave
	SSP1ADD = I2C_BTNICSLAVE_ADDR;

	PIR1bits.SSP1IF = 0;
	PIE1bits.SSP1IE = 1; 
	return 0;
}

//Called by ISR
void BTCommRX(void)
{
	if (SSP1CON1bits.SSPOV) {	//Check for overflow
		ReadI2C1();		//Do a dummy read
		SSP1CON1bits.SSPOV = 0;	//Clear the overflow flag
	}
	else {
		switch (SSP1STAT & I2C_SLAVESTATE_BITMASK)
		{
			case I2C_SLAVESTATE_WRITE_ADDR:
				ReadI2C1();	//Dummy read of address
				break;
			case I2C_SLAVESTATE_WRITE_DATA:
				if (BTCommState == BT_COMMSTATE_WAIT)
				{
					BTCommSetState(BT_COMMSTATE_RX);
					BTCommBuffer[BTCommLen++] = '[';
					BTCommBuffer[BTCommLen++] = '"';
				}
				else if (BTCommState == BT_COMMSTATE_IDLE || BTCommState == BT_COMMSTATE_TX)
				{
					BTCommSetState(BT_COMMSTATE_ASYNCRX);
					BTCommBuffer[BTCommLen++] = '[';
					BTCommBuffer[BTCommLen++] = '"';
				}
				if (BTCommState == BT_COMMSTATE_RX || BTCommState == BT_COMMSTATE_ASYNCRX) {
					unsigned char byteIn = ReadI2C1();
					switch (byteIn) {
						case '\t':
							//End Field/Start New
							BTCommBuffer[BTCommLen++] = '"';
							BTCommBuffer[BTCommLen++] = ',';
							BTCommBuffer[BTCommLen++] = '"';
							break;
						case '\n':
							//Ignore New Line (Workaround: Not getting this on I2C TX so processing on Carriage Return for now)
							break;
						case '\r':
							//End Field & Message
							BTCommBuffer[BTCommLen++] = '"';
							BTCommBuffer[BTCommLen++] = ']';
							if (BTCommState == BT_COMMSTATE_RX) BTCommSetState(BT_COMMSTATE_MSG);
							else BTCommSetState(BT_COMMSTATE_ASYNCMSG);
							break;
						default:
							BTCommBuffer[BTCommLen++] = byteIn;
							break;
					}
				}
				break;
			case I2C_SLAVESTATE_READ_ADDR:
				//Not implemented
				break;
			case I2C_SLAVESTATE_READ_DATA:
				//Not implemented
				break;
			case I2C_SLAVESTATE_NACK:
				// Reset the SSP Unit
				SSP1CON1 = 0x00;
				SSP1CON1 = 0x26;  //I2C 7-Bit Slave
				SSP1CON2 = 0x00;
				SSP1ADD = I2C_BTNICSLAVE_ADDR;
				break;
		}
	}	
	PIR1bits.SSP1IF = 0; //Clear SSPIF Interrupt Flag
}

char BTCommGetState() 
{ 
	int timeout = 0;
	if 		(BTCommState == BT_COMMSTATE_TX) timeout = BT_TIMEOUT_TX;
	else if (BTCommState == BT_COMMSTATE_WAIT) timeout = BT_TIMEOUT_WAIT;
	else if (BTCommState == BT_COMMSTATE_RX) timeout = BT_TIMEOUT_RX;
	else if (BTCommState == BT_COMMSTATE_MSG) timeout = BT_TIMEOUT_MSG;

	if (timeout && TickGet() - BTCommTimer > (timeout * (TICK_SECOND / 1000))) BTCommSetState(BT_COMMSTATE_IDLE);

	//TO DO: ASYNCMSG Processing
	if (BTCommState == BT_COMMSTATE_ASYNCMSG) BTCommSetState(BT_COMMSTATE_IDLE);

	return BTCommState; 
}

void BTCommSetState(char state)
{
	BTCommState = state;
	BTCommTimer = TickGet();
	if (state == BT_COMMSTATE_IDLE) BTCommLen = BTCommCur = 0;
}

unsigned int BTCommGetRspLen() { return BTCommLen; }

char BTCommGetRsp()
{
	char byteOut;
	//if (BTCommState != BT_COMMSTATE_MSG && BTCommState != BT_COMMSTATE_ASYNCMSG) return '\0';
	//Last byte state handling
	byteOut = BTCommBuffer[BTCommCur++];
	if (BTCommCur == BTCommLen) BTCommSetState(BT_COMMSTATE_IDLE);
	return byteOut;
}

unsigned long BTCommGetTimer()
{
	return BTCommTimer;
}

void BTCommSetRsp(far rom char* data)
{
	strcpypgm2ram(BTCommBuffer, data);
	BTCommLen = strlen(BTCommBuffer);
	BTCommSetState(BT_COMMSTATE_MSG);
}

char BTCommGetBuffer(unsigned int index)
{
	return BTCommBuffer[index];
}