#ifndef SRAM_COMM
#define SRAM_COMM

#include "HardwareProfile.h"
#include "sw_spi.h"
#include <delays.h>
#include "GenericTypeDefs.h"

#define SRAM_CMD_READ				0x03
#define SRAM_CMD_WRITE				0x02
#define SRAM_CMD_RDSR				0x05
#define SRAM_CMD_WRSR				0x01

void sramReadBytes(char *dest, unsigned int addr, unsigned char len);
char sramReadByte(unsigned int addr);
void sramWriteBytes(char *dest, unsigned int addr, unsigned char len);
void sramWriteByte(char data, unsigned int addr);
char sramRDSR(void);
void sramWRSR(char data);
void sramAddr(unsigned int addr);
void sramDisable(void);
void sramEnable(void);

#endif