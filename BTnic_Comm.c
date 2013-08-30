#include "BTnic_Comm.h"
#include "TCPIP Stack/Tick.h"
#include "Main.h"

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
	BTCommSetState(COMMSTATE_IDLE);

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
	if (BTCommState != COMMSTATE_IDLE) return(1);
	BTCommSetState(COMMSTATE_BUFFERING);
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
	BTCommSetState(COMMSTATE_TXREADY);
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
			if (BTCommState == COMMSTATE_TXREADY) 
				BTCommSetState(COMMSTATE_TX);

			while(SSP1STATbits.BF);      //wait while buffer is full 
		    do { 
		        SSP1CON1bits.WCOL = 0;           //clear write collision flag 
		        SSP1BUF = BTCommState == COMMSTATE_TX ? BTCommTXBuffer[BTCommTXCursor] : 0; 
		    } while (SSP1CON1bits.WCOL);           //do until write collision flag is clear

			if (SSP1CON2bits.SEN)
				SSP1CON1bits.CKP = 1;           //release the SCL line 
			
			if (BTCommState == COMMSTATE_TX) {
				BTCommTXCursor++;
				if (BTCommTXCursor >= BTCommTXLen)
					BTCommSetState(COMMSTATE_WAIT);
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
			if (BTCommState == COMMSTATE_WAIT)
				BTCommSetState(COMMSTATE_RX);

			if (BTCommState == COMMSTATE_RX) {
				byteIn = SSP1BUF;
				switch (byteIn) {
					case '\n':
						//Ignore New Line (Workaround: Not getting this on I2C TX so processing on Carriage Return for now)
						break;
					case '\r':
						//End Field & Message
						if (BTCommState == COMMSTATE_RX)
						{
							BTCommSetState(COMMSTATE_MSG);
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
	if (WebSrvConfig.StateTimeout[BTCommState]) {
		if ((TickGet() - BTCommTimer) > (WebSrvConfig.StateTimeout[BTCommState] * (TICK_SECOND / 1000)))
			BTCommInit(); //Reset state and I2C Bus
	}

	//TO DO: ASYNCMSG Processing
	//if (BTCommState == COMMSTATE_ASYNCMSG) BTCommSetState(COMMSTATE_IDLE);
	return BTCommState; 
}

void BTCommSetState(char state)
{
	BTCommState = state;
	BTCommTimer = TickGet();
	if (state == COMMSTATE_IDLE) BTCommRXLen = BTCommRXCursor = BTCommTXLen = BTCommTXCursor = 0;
}

unsigned int BTCommGetRspLen() { return BTCommRXLen; }
unsigned int BTCommGetReqLen() { return BTCommTXLen; }

char BTCommGetRsp()
{
	char byteOut;
	if (BTCommState != COMMSTATE_MSG) return '\0';
	//Last byte state handling
	byteOut = BTCommRXBuffer[BTCommRXCursor++];
	if (BTCommRXCursor == BTCommRXLen) BTCommSetState(COMMSTATE_IDLE);
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
	BTCommSetState(COMMSTATE_MSG);
}

char BTCommGetRspBuffer(unsigned int index)
{
	return BTCommRXBuffer[index];
}

char BTCommGetReqBuffer(unsigned int index)
{
	return BTCommTXBuffer[index];
}