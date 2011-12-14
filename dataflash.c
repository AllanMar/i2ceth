#include "dataflash.h"

void dataflashReadBytes(char *dest, unsigned short long addr, unsigned char len)
{
	char *pDest = dest;
	while(dataflashBusy());
	dataflashEnable();
	WriteSWSPI( DATAFLASH_CMD_READ );
	dataflashAddr( addr );

	while (len--) *pDest++ = ReadSWSPI();
	dataflashDisable();
}

char dataflashReadByte(unsigned short long addr)
{
	char data;
	while(dataflashBusy());
	dataflashEnable();
	WriteSWSPI( DATAFLASH_CMD_READ );
	dataflashAddr( addr );
	data = ReadSWSPI();
	dataflashDisable();
	return data;
}

void dataflashWriteBytes(char *data, unsigned short long addr, unsigned char len)
{
	unsigned char sendAddr = 1;
	char *pData = data;
	while(dataflashBusy());
	while (len) {
		dataflashWREN();
		while (len) {
			dataflashEnable();
			WriteSWSPI( DATAFLASH_CMD_AAIWORDPGM );
			if (sendAddr) { sendAddr = 0; dataflashAddr( addr ); }
			WriteSWSPI(*pData++); WriteSWSPI(*pData++); len-=2;
			dataflashDisable();
			while(dataflashBusy());
		}
		dataflashEnable();
		WriteSWSPI( DATAFLASH_CMD_WRDI );
		dataflashDisable();
	}	
}

void dataflashWriteByte(char data, unsigned short long addr)
{
	while(dataflashBusy());
	dataflashWREN();
	dataflashEnable();
	WriteSWSPI( DATAFLASH_CMD_BYTEPGM );
	dataflashAddr( addr );
	WriteSWSPI(data);
	dataflashDisable();
}

void dataflashSectorErase(unsigned short long addr)
{
	while(dataflashBusy());
	dataflashWREN();
	dataflashEnable();
	WriteSWSPI( DATAFLASH_CMD_4KBERASE );
	dataflashAddr( addr );
	dataflashDisable();
}

char dataflashBusy(void) { return (dataflashRDSR() & 1); }

char dataflashRDSR(void)
{
	char data, dummy;
	dataflashEnable();
	WriteSWSPI( DATAFLASH_CMD_RDSR );
	data = ReadSWSPI();
	dummy = data;
	dataflashDisable();
	return data;
}

void dataflashWRSR(char data)
{
	while(dataflashBusy());
	dataflashWREN();
	dataflashEnable();
	WriteSWSPI( DATAFLASH_CMD_WRSR );
	WriteSWSPI(data);
	dataflashDisable();
}

void dataflashAddr(unsigned short long addr)
{
		WriteSWSPI( addr >> 16u );
		WriteSWSPI( (addr >> 8u) & 255u );
		WriteSWSPI( addr & 255u );
}

void dataflashWREN() {
	dataflashEnable();
	WriteSWSPI( DATAFLASH_CMD_WREN );
	dataflashDisable();
}

void dataflashDisable() {
	Nop(); Nop();
	SW_CS2_PIN = 1;
	Nop(); Nop();
}

void dataflashEnable() {
	Nop(); Nop();
	SW_CS2_PIN = 0;
	Nop(); Nop();
}
