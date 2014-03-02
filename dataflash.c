#include "dataflash.h"

// Internal pointer to address being written
DWORD dwWriteAddr;

void SPIFlashInit()
{
}

void SPIFlashReadBytes(unsigned short long addr, char *dest, unsigned char len)
{
	char *pDest = dest;
	while(SPIFlashBusy());
	SPIFlashEnable();
	WriteSWSPI( DATAFLASH_CMD_READ );
	SPIFlashAddr( addr );

	while (len--) *pDest++ = ReadSWSPI();
	SPIFlashDisable();
}

char SPIFlashReadByte(unsigned short long addr)
{
	char data;
	while(SPIFlashBusy());
	SPIFlashEnable();
	WriteSWSPI( DATAFLASH_CMD_READ );
	SPIFlashAddr( addr );
	data = ReadSWSPI();
	SPIFlashDisable();
	return data;
}

void SPIFlashBeginWrite(DWORD dwAddr)
{
    dwWriteAddr = dwAddr;
}

void SPIFlashWriteByte(char data)
{
	while(SPIFlashBusy());
	SPIFlashWREN();
	SPIFlashEnable();
	WriteSWSPI( DATAFLASH_CMD_BYTEPGM );
	SPIFlashAddr( dwWriteAddr );
	WriteSWSPI(data);
	SPIFlashDisable();
}

void SPIFlashWriteBytes(char *data, unsigned char len)
{
	unsigned char sendAddr = 1;
	char *pData = data;
	while(SPIFlashBusy());
	while (len) {
		SPIFlashWREN();
		while (len) {
			SPIFlashEnable();
			WriteSWSPI( DATAFLASH_CMD_AAIWORDPGM );
			if (sendAddr) { sendAddr = 0; SPIFlashAddr( dwWriteAddr ); }
			WriteSWSPI(*pData++); WriteSWSPI(*pData++); len-=2;
			SPIFlashDisable();
			while(SPIFlashBusy());
		}
		SPIFlashEnable();
		WriteSWSPI( DATAFLASH_CMD_WRDI );
		SPIFlashDisable();
	}	
}

void SPIFlashEraseSector(unsigned short long addr)
{
	while(SPIFlashBusy());
	SPIFlashWREN();
	SPIFlashEnable();
	WriteSWSPI( DATAFLASH_CMD_4KBERASE );
	SPIFlashAddr( addr );
	SPIFlashDisable();
}

char SPIFlashBusy(void) { return (SPIFlashRDSR() & 1); }

char SPIFlashRDSR(void)
{
	char data, dummy;
	SPIFlashEnable();
	WriteSWSPI( DATAFLASH_CMD_RDSR );
	data = ReadSWSPI();
	dummy = data;
	SPIFlashDisable();
	return data;
}

void SPIFlashWRSR(char data)
{
	while(SPIFlashBusy());
	SPIFlashWREN();
	SPIFlashEnable();
	WriteSWSPI( DATAFLASH_CMD_WRSR );
	WriteSWSPI(data);
	SPIFlashDisable();
}

void SPIFlashAddr(unsigned short long addr)
{
		WriteSWSPI( addr >> 16u );
		WriteSWSPI( (addr >> 8u) & 255u );
		WriteSWSPI( addr & 255u );
}

void SPIFlashWREN() {
	SPIFlashEnable();
	WriteSWSPI( DATAFLASH_CMD_WREN );
	SPIFlashDisable();
}

void SPIFlashDisable() {
	Nop(); Nop();
	SW_CS2_PIN = 1;
	Nop(); Nop();
}

void SPIFlashEnable() {
	Nop(); Nop();
	SW_CS2_PIN = 0;
	Nop(); Nop();
}
