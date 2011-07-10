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

#define BT_COMMSTATE_IDLE		0x00 //Connection available
#define BT_COMMSTATE_WAIT		0x03 //Waiting for response
#define BT_COMMSTATE_RX			0x04 //Receiving response
#define BT_COMMSTATE_MSG	0x05 //Response in buffer
#define BT_COMMSTATE_ASYNCRX	0x06 //Receiving unsolicited message
#define BT_COMMSTATE_ASYNCMSG	0x07 //Unsolicited message in buffer

#define SM_BTNIC_WAIT_TO_SEND				(0u)
#define SM_BTNIC_WAIT_FOR_RESP				(1u)

void BTCommInit(void);
unsigned char BTCommTX(unsigned char*);
void BTCommRX(void);
unsigned char BTCommGetStatus(void);
unsigned int BTCommGetRspLen(void);
unsigned char BTCommGetRsp(void);
	
#endif