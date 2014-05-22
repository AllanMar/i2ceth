#ifndef BTNIC_COMM
#define BTNIC_COMM

#include "HardwareProfile.h"

//#define I2C_BTSLAVE_ADDR		0x20 //0x10 7-Bit address with LSB = 0
#define I2C_BTNICSLAVE_ADDR		0x22 //0x11 7-bit address with LSB = 0
//#define I2C_MASTER_BAUDRATE		0x77 //100kHz @ 41.667
#define I2C_NO_SLEW_OR_SMBUS	0x80
#define I2C_SLAVE_7BIT			0x36
//#define I2C_MASTER				0x28
	
//Slave States
//#define I2C_SLAVESTATE_BITMASK  0b00101101 // Mask for I2C status bits

//#define I2C_SLAVESTATE_WRITE_ADDR	0b00001001 //D_A = 0, S = 1, R_W = 0, BF = 1
//#define I2C_SLAVESTATE_WRITE_DATA	0b00101001 //D_A = 1, S = 1, R_W = 0, BF = 1
//#define I2C_SLAVESTATE_READ_ADDR	0b00001101 //D_A = 1, S = 1, R_W = 1, BF = 1
//#define I2C_SLAVESTATE_READ_DATA	0b00101101 //D_A = 1, S = 1, R_W = 1, BF = 1
//#define I2C_SLAVESTATE_NACK			0b00101000 //D_A = 1, S = 1, R_W = 0, BF = 0

#define I2C_SLAVESTATE_BITMASK  0b00101101 // Mask for I2C status bits
#define I2C_SLAVESTATE_WRITE_ADDR	0b00001001 //D_A = 0, S = 1, R_W = 0, BF = 1
#define I2C_SLAVESTATE_WRITE_DATA	0b00101001 //D_A = 1, S = 1, R_W = 0, BF = 1
#define I2C_SLAVESTATE_READ_ADDR	0b00001101 //D_A = 0, S = 1, R_W = 1, BF = 1
#define I2C_SLAVESTATE_READ_DATA	0b00101100 //D_A = 1, S = 1, R_W = 1, BF = 0
#define I2C_SLAVESTATE_NACK			0b00101000 //D_A = 1, S = 1, R_W = 0, BF = 0

#define BTCOMM_RXBUFFER_SIZE 1024
#define BTCOMM_TXBUFFER_SIZE 200

#define BT_BUFFER_RESPONSE	0
#define BT_BUFFER_REQUEST 	1

enum CommunicationState {
	COMMSTATE_IDLE,			//Ready for connection
	COMMSTATE_BUFFERING,	//Lock buffer for use
	COMMSTATE_TXREADY,		//Request in buffer
	COMMSTATE_TX,			//Sending request
	COMMSTATE_WAIT,			//Waiting for response
	COMMSTATE_RX,			//Receiving response
	COMMSTATE_MSG,			//Response in buffer
	NUM_COMMSTATES
};

#define SM_BTNIC_START			0
#define SM_BTNIC_TX_RETRY		1
#define SM_BTNIC_WAIT_FOR_RESP	2

void BTCommInit(void);
int BTCommRequest(char*);
void BTCommRX(void);
char BTCommGetState(void);
void BTCommSetState(char);
unsigned int BTCommGetRspLen(void);
unsigned int BTCommGetReqLen(void);
char BTCommGetRsp(void);
char BTCommGetRspBuffer(unsigned int);
char BTCommGetReqBuffer(unsigned int);
unsigned long BTCommGetTimer(void);
void BTCommSetRsp(far rom char*);
	
#endif