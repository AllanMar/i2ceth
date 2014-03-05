#include "dataflash.h"

// Internal pointer to address being written
DWORD dwWriteAddr;

void SPIFlashInit()
{
	SPIFlashWRDI();
	SPIFlashWRSR(0);
}

void SPIFlashReadArray(unsigned short long addr, char *dest, unsigned char len)
{
	while(SPIFlashBusy());
	SPIFlashEnable();
	WriteSWSPI( DATAFLASH_CMD_READ );
	SPIFlashAddr( addr );
	while (len--) *dest++ = WriteSWSPI(0);
	SPIFlashDisable();
}

char SPIFlashReadByte(unsigned short long addr)
{
	char data;
	while(SPIFlashBusy());
	SPIFlashEnable();
	WriteSWSPI( DATAFLASH_CMD_READ );
	SPIFlashAddr( addr );
	data = WriteSWSPI(0);
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

void SPIFlashWriteArray(char *data, unsigned char len)
{
	unsigned char sendAddr = 1;

	// If address is 0 start with chip erase
    if((dwWriteAddr) == 0)
    	SPIFlashChipErase();

	SPIFlashWREN();

	while (len > 1) {
		while(SPIFlashBusy());
		SPIFlashEnable();
		WriteSWSPI( DATAFLASH_CMD_AAIWORDPGM );
		if (sendAddr) {
			SPIFlashAddr( dwWriteAddr );
			sendAddr = 0;
		}
		WriteSWSPI(*data++);
		WriteSWSPI(*data++);
		len -= 2;
		dwWriteAddr += 2;
		SPIFlashDisable();
		while(SPIFlashBusy());
	}
	SPIFlashWRDI();
}

void SPIFlashChipErase()
{
	while(SPIFlashBusy());
	SPIFlashWREN();
	SPIFlashEnable();
	WriteSWSPI( DATAFLASH_CMD_CHIPERASE );
	SPIFlashDisable();
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
	char data;
	SPIFlashEnable();
	WriteSWSPI( DATAFLASH_CMD_RDSR );
	data = WriteSWSPI(0);
	SPIFlashDisable();
	return data;
}

void SPIFlashWRSR(char data)
{
	while(SPIFlashBusy());
	SPIFlashEWSR();
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
	while(SPIFlashBusy());
	SPIFlashEnable();
	WriteSWSPI( DATAFLASH_CMD_WREN );
	SPIFlashDisable();
}

void SPIFlashEWSR() {
	SPIFlashEnable();
	WriteSWSPI( DATAFLASH_CMD_EWSR );
	SPIFlashDisable();
}

void SPIFlashWRDI() {
	SPIFlashEnable();
	WriteSWSPI( DATAFLASH_CMD_WRDI );
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
