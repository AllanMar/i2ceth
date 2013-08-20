#include "BTnic_Comm.h"
#include <i2c.h>
#include "TCPIP Stack/Tick.h"

#pragma udata BUFFERS // section BUFFERS
volatile char BTCommRXBuffer[BTCOMM_RXBUFFER_SIZE];
volatile char BTCommTXBuffer[BTCOMM_TXBUFFER_SIZE];

#pragma udata // return to default section
volatile unsigned int BTCommRXLen, BTCommRXCursor, BTCommTXLen, BTCommTXCursor;
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

	SSP1STAT = I2C_NO_SLEW_OR_SMBUS;   //Disable SMBus & Slew Rate Control
 	SSP1CON1 = I2C_SLAVE_7BIT;  //I2C 7-Bit Slave

 	//SSPCON2 |= 1;        // Clock stretching is enabled
	SSP1ADD = I2C_BTNICSLAVE_ADDR;
	PIE1bits.SSP1IE = 1; 

	//Enable I2C interrupts    
	INTCONbits.PEIE = 1;          //Turn on peripheral interrupts    
	INTCONbits.GIE = 1;           //Turn on global interrupts
}

//Insert request into buffer
int BTCommRequest(char* reqMsg)
{
	if (BTCommState != BT_COMMSTATE_IDLE) return ( -1 );
	while (1) {
	    while(*reqMsg != '\0') 
		{
			if (*reqMsg == '+') *reqMsg = ' ';
			BTCommTXBuffer[BTCommTXLen++] = *reqMsg++;
	    }
		if (*(reqMsg + 1) == '\0') break; //Double Null: End of Request
		*reqMsg = '\t'; //Single Null: Field Delim
	}
	BTCommTXBuffer[BTCommTXLen++] ='\r';
	BTCommSetState(BT_COMMSTATE_TXREADY);
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
				}
				else if (BTCommState == BT_COMMSTATE_IDLE || BTCommState == BT_COMMSTATE_TX || BTCommState == BT_COMMSTATE_TXREADY)
				{
					BTCommSetState(BT_COMMSTATE_ASYNCRX);
				}
				if (BTCommState == BT_COMMSTATE_RX || BTCommState == BT_COMMSTATE_ASYNCRX) {
					unsigned char byteIn = ReadI2C1();
					switch (byteIn) {
						case '\n':
							//Ignore New Line (Workaround: Not getting this on I2C TX so processing on Carriage Return for now)
							break;
						case '\r':
							//End Field & Message
							if (BTCommState == BT_COMMSTATE_RX) BTCommSetState(BT_COMMSTATE_MSG);
							else BTCommSetState(BT_COMMSTATE_ASYNCMSG);
							break;
						default:
							BTCommRXBuffer[BTCommRXLen++] = byteIn;
							break;
					}
				}
				break;
			case I2C_SLAVESTATE_READ_ADDR:
				ReadI2C1();	//Dummy read of address
			case I2C_SLAVESTATE_READ_DATA:
				if (BTCommState == BT_COMMSTATE_TXREADY)
					BTCommSetState(BT_COMMSTATE_TX);
				if (BTCommState == BT_COMMSTATE_TX)
				{
					WriteI2C1(BTCommTXBuffer[BTCommTXCursor++]);
					if (BTCommTXCursor >= BTCommTXLen)
						BTCommSetState(BT_COMMSTATE_WAIT);
				}
				else
					WriteI2C1(0);
					//ReadI2C1(); //Dummy Read
					//SSPCON1bits.CKP = 1; 
				break;
			default:
				// Reset the SSP Unit
				SSP1CON1 = 0x00;
				SSP1CON1 = I2C_SLAVE_7BIT;  //I2C 7-Bit Slave
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
	if 		(BTCommState == BT_COMMSTATE_TXREADY) timeout = BT_TIMEOUT_TXREADY;
	else if (BTCommState == BT_COMMSTATE_TX) timeout = BT_TIMEOUT_TX;
	else if (BTCommState == BT_COMMSTATE_WAIT) timeout = BT_TIMEOUT_WAIT;
	else if (BTCommState == BT_COMMSTATE_RX) timeout = BT_TIMEOUT_RX;
	else if (BTCommState == BT_COMMSTATE_MSG) timeout = BT_TIMEOUT_MSG;

	if (timeout && TickGet() - BTCommTimer > (timeout * (TICK_SECOND / 1000))) BTCommInit(); //Reset state and I2C Bus

	//TO DO: ASYNCMSG Processing
	if (BTCommState == BT_COMMSTATE_ASYNCMSG) BTCommSetState(BT_COMMSTATE_IDLE);

	return BTCommState; 
}

void BTCommSetState(char state)
{
	BTCommState = state;
	BTCommTimer = TickGet();
	if (state == BT_COMMSTATE_IDLE) BTCommRXLen = BTCommRXCursor = BTCommTXLen = BTCommTXCursor = 0;
}

unsigned int BTCommGetRspLen() { return BTCommRXLen - BTCommRXCursor; }

unsigned int BTCommGetRspCount() 
{ 
	//Temporary hack. RspCount is 1 or 0 until multi-cmd/rsp support implemented
	return (BTCommRXLen > 0 && (BTCommState == BT_COMMSTATE_MSG || BTCommState == BT_COMMSTATE_ASYNCMSG) ? 1 : 0);
}

char BTCommGetRsp()
{
	char byteOut;
	//if (BTCommState != BT_COMMSTATE_MSG && BTCommState != BT_COMMSTATE_ASYNCMSG) return '\0';
	//Last byte state handling
	byteOut = BTCommRXBuffer[BTCommRXCursor++];
	if (BTCommRXCursor == BTCommRXLen) BTCommSetState(BT_COMMSTATE_IDLE);
	return byteOut;
}

unsigned long BTCommGetTimer()
{
	return BTCommTimer;
}

void BTCommSetRsp(far rom char* data)
{
	strcpypgm2ram(BTCommRXBuffer, data);
	BTCommRXLen = strlen(BTCommRXBuffer);
	BTCommSetState(BT_COMMSTATE_MSG);
}

char BTCommGetBuffer(unsigned int index)
{
	return BTCommRXBuffer[index];
}