#ifndef BTNIC_COMM
#define BTNIC_COMM

#include "HardwareProfile.h"

#define I2C_BTSLAVE_ADDR		0x20 //0x10 7-Bit address with LSB = 0
#define I2C_BTNICSLAVE_ADDR		0x22 //0x11 7-bit address with LSB = 0
#define I2C_MASTER_BAUDRATE		0x77 //100kHz @ 41.667
#define I2C_NO_SLEW_OR_SMBUS	0x80
#define I2C_SLAVE_7BIT			0x36
#define I2C_MASTER				0x28
	
//Slave States
#define I2C_SLAVESTATE_BITMASK  0b00101101 // Mask for I2C status bits

#define I2C_SLAVESTATE_WRITE_ADDR	0b00001001 //S = 1, D_A = 0, R_W = 0, BF = 1
#define I2C_SLAVESTATE_WRITE_DATA	0b00101001 //S = 1, D_A = 1, R_W = 0, BF = 1
#define I2C_SLAVESTATE_READ_ADDR	0b00001100 //S = 1, D_A = 0, R_W = 1, BF = 0
#define I2C_SLAVESTATE_READ_DATA	0b00101100 //S = 1, D_A = 1, R_W = 1, BF = 0
#define I2C_SLAVESTATE_NACK			0b00101000 //S = 1, D_A = 1, R_W = 0, BF = 0

//Note: Larger values cause a link error. See: http://www.microchip.com/forums/tm.aspx?m=310090
#define BTCOMM_BUFFER_SIZE 240

#define BT_COMMSTATE_IDLE		0 //Connection available
#define BT_COMMSTATE_TX			1 //Sending request
#define BT_COMMSTATE_WAIT		3 //Waiting for response
#define BT_COMMSTATE_RX			4 //Receiving response
#define BT_COMMSTATE_MSG		5 //Response in buffer
#define BT_COMMSTATE_ASYNCRX	6 //Receiving unsolicited message
#define BT_COMMSTATE_ASYNCMSG	7 //Unsolicited message in buffer

#define SM_BTNIC_START			0
#define SM_BTNIC_TX_RETRY		1
#define SM_BTNIC_WAIT_FOR_RESP	2

#define BT_TIMEOUT_TX 	5000 //ms
#define BT_TIMEOUT_WAIT 20000 //ms
#define BT_TIMEOUT_RX 	20000 //ms
#define BT_TIMEOUT_MSG	5000 //ms

void BTCommInit(void);
int BTCommTX(char*);
void BTCommRX(void);
char BTCommGetState(void);
void BTCommSetState(char);
unsigned int BTCommGetRspLen(void);
unsigned int BTCommGetRspCount(void);
char BTCommGetRsp(void);
char BTCommGetBuffer(unsigned int);
unsigned long BTCommGetTimer(void);
void BTCommSetRsp(far rom char*);
	
#endif