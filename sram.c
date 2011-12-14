#include "sram.h"

void sramReadBytes(char *dest, unsigned int addr, unsigned char len)
{
	char *pDest = dest;
	sramWRSR(65u); //Set sequential mode with HOLD disabled
	sramEnable();
	WriteSWSPI( SRAM_CMD_READ );
	sramAddr( addr );
	while (len--) *pDest++ = ReadSWSPI();
	sramDisable();
}

char sramReadByte(unsigned int addr)
{
	char data;
	sramWRSR(1u); //Set byte mode with HOLD disabled
	sramEnable();
	WriteSWSPI( SRAM_CMD_READ );
	sramAddr( addr );
	data = ReadSWSPI();
	sramDisable();
	return data;
}

void sramWriteBytes(char *data, unsigned int addr, unsigned char len)
{
	char *pData = data;
	sramWRSR(65u); //Set sequential mode with HOLD disabled
	sramEnable();
	WriteSWSPI( SRAM_CMD_WRITE );
	sramAddr( addr );
	while (len--) WriteSWSPI(*pData++);
	sramDisable();
}

void sramWriteByte(char data, unsigned int addr)
{
	sramWRSR(1u); //Set byte mode with HOLD disabled
	sramEnable();
	WriteSWSPI( SRAM_CMD_WRITE );
	sramAddr( addr );
	WriteSWSPI(data);
	sramDisable();
}

char sramRDSR(void)
{
	char data;
	sramEnable();
	WriteSWSPI( SRAM_CMD_RDSR );
	data = ReadSWSPI();
	sramDisable();
	return data;
}

void sramWRSR(char data)
{
	sramEnable();
	WriteSWSPI( SRAM_CMD_WRSR );
	WriteSWSPI(data);
	sramDisable();
}

void sramAddr(unsigned int addr)
{
	WriteSWSPI( addr >> 8u );
	WriteSWSPI( addr & 255u );
}

void sramDisable() {
	Nop(); Nop();
	SW_CS3_PIN = 1;
	Nop(); Nop();
}

void sramEnable() {
	Nop(); Nop();
	SW_CS3_PIN = 0;
	Nop(); Nop();
}
