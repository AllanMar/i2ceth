#include "BTnic_Comm.h"
#include "TCPIP Stack/Tick.h"

#pragma udata BUFFERS // section BUFFERS
volatile char BTCommRXBuffer[BTCOMM_RXBUFFER_SIZE];
volatile char BTCommTXBuffer[BTCOMM_TXBUFFER_SIZE];

#pragma udata // return to default section
volatile unsigned int BTCommRXLen, BTCommRXCursor, BTCommTXLen, BTCommTXCursor;
volatile BYTE BTCommState;
unsigned char BTCommDebug;
volatile unsigned long BTCommTimer;

void BTCommInit(unsigned char debugMode)
{
	BTCommDebug = debugMode;

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

 	SSPCON2bits.SEN = 1;        // Clock stretching is enabled
	SSP1ADD = I2C_BTNICSLAVE_ADDR;
	PIE1bits.SSP1IE = 1; 

	//Enable I2C interrupts    
	INTCONbits.PEIE = 1;          //Turn on peripheral interrupts    
	INTCONbits.GIE = 1;           //Turn on global interrupts
}

//Insert request into buffer
int BTCommRequest(char* reqMsg)
{
	if (BTCommState != BT_COMMSTATE_IDLE) return(1);
	BTCommSetState(BT_COMMSTATE_BUFFERING);
	BTCommTXBuffer[BTCommTXLen++] = 0x01; //Start of message (reset BT request buffer)
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
	unsigned char byteIn;
	switch (SSP1STAT & I2C_SLAVESTATE_BITMASK) {
		case I2C_SLAVESTATE_READ_ADDR:
			byteIn = SSP1BUF;	//Dummy read of address

		case I2C_SLAVESTATE_READ_DATA:
			if (BTCommState == BT_COMMSTATE_TXREADY) 
				BTCommSetState(BT_COMMSTATE_TX);

			while(SSP1STATbits.BF);      //wait while buffer is full 
		    do { 
		        SSP1CON1bits.WCOL = 0;           //clear write collision flag 
		        SSP1BUF = BTCommState == BT_COMMSTATE_TX ? BTCommTXBuffer[BTCommTXCursor] : 0; 
		    } while (SSP1CON1bits.WCOL);           //do until write collision flag is clear

			if (SSP1CON2bits.SEN)
				SSP1CON1bits.CKP = 1;           //release the SCL line 
			
			if (BTCommState == BT_COMMSTATE_TX) {
				BTCommTXCursor++;
				if (BTCommTXCursor >= BTCommTXLen)
					BTCommSetState(BT_COMMSTATE_WAIT);
			}
			break;

		case I2C_SLAVESTATE_WRITE_ADDR:
           	byteIn = SSP1BUF;	//Dummy read of address
			if(SSP1CON1bits.SSPOV)
				SSP1CON1bits.SSPOV = 0;              //clear receive overflow indicator 
			if (SSP1CON2bits.SEN)
				SSP1CON1bits.CKP = 1;           //release the SCL line 
			break;

		case I2C_SLAVESTATE_WRITE_DATA:
			if (BTCommState == BT_COMMSTATE_WAIT)
				BTCommSetState(BT_COMMSTATE_RX);

			if (BTCommState == BT_COMMSTATE_RX) {
				byteIn = SSP1BUF;
				switch (byteIn) {
					case '\n':
						//Ignore New Line (Workaround: Not getting this on I2C TX so processing on Carriage Return for now)
						break;
					case '\r':
						//End Field & Message
						if (BTCommState == BT_COMMSTATE_RX)
						{
							BTCommSetState(BT_COMMSTATE_MSG);
						}
						break;
					default:
						BTCommRXBuffer[BTCommRXLen++] = byteIn;
						break;
				}
			}
			if (SSP1CON2bits.SEN)
				SSP1CON1bits.CKP = 1;           //release the SCL line 
			break;
		case I2C_SLAVESTATE_NACK:
			break;
		default: 
			if (SSP1CON2bits.SEN)
				SSP1CON1bits.CKP = 1;           //release the SCL line 
			break; 
	}	
}

char BTCommGetState() 
{ 

	int timeout = 0;
	if 		(BTCommState == BT_COMMSTATE_BUFFERING) timeout = BT_TIMEOUT_BUFFERING;
	else if (BTCommState == BT_COMMSTATE_TXREADY) timeout = BT_TIMEOUT_TXREADY;
	else if (BTCommState == BT_COMMSTATE_TX) timeout = BT_TIMEOUT_TX;
	else if (BTCommState == BT_COMMSTATE_WAIT) timeout = BT_TIMEOUT_WAIT;
	else if (BTCommState == BT_COMMSTATE_RX) timeout = BT_TIMEOUT_RX;
	else if (BTCommState == BT_COMMSTATE_MSG) timeout = BT_TIMEOUT_MSG;

	if (!BTCommDebug && timeout) {
		if ((TickGet() - BTCommTimer) > (timeout * (TICK_SECOND / 1000)))
			BTCommInit(BTCommDebug); //Reset state and I2C Bus
	}

	//TO DO: ASYNCMSG Processing
	//if (BTCommState == BT_COMMSTATE_ASYNCMSG) BTCommSetState(BT_COMMSTATE_IDLE);
	return BTCommState; 
}

void BTCommSetState(char state)
{
	BTCommState = state;
	BTCommTimer = TickGet();
	if (state == BT_COMMSTATE_IDLE) BTCommRXLen = BTCommRXCursor = BTCommTXLen = BTCommTXCursor = 0;
}

unsigned int BTCommGetRspLen() { return BTCommRXLen; }
unsigned int BTCommGetReqLen() { return BTCommTXLen; }

unsigned int BTCommGetRspCount() 
{ 
	//Temporary hack. RspCount is 1 or 0 until multi-cmd/rsp support implemented
	return (BTCommRXLen > 0 && (BTCommState == BT_COMMSTATE_MSG || BTCommState == BT_COMMSTATE_ASYNCMSG) ? 1 : 0);
}

char BTCommGetRsp()
{
	char byteOut;
	if (BTCommState != BT_COMMSTATE_MSG) return '\0';
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

char BTCommGetRspBuffer(unsigned int index)
{
	return BTCommRXBuffer[index];
}

char BTCommGetReqBuffer(unsigned int index)
{
	return BTCommTXBuffer[index];
}