#include "TCPIP Stack/SPIFlash.h"

#include "sw_spi.h"
//#include <delays.h>

#define DATAFLASH_CMD_READ				0x03
#define DATAFLASH_CMD_HIGHSPEEDREAD		0x0B
#define DATAFLASH_CMD_4KBERASE			0x20
#define DATAFLASH_CMD_32KBERASE			0x52
#define DATAFLASH_CMD_64KBERASE			0xD8
#define DATAFLASH_CMD_CHIPERASE			0x60
#define DATAFLASH_CMD_BYTEPGM			0x02
#define DATAFLASH_CMD_AAIWORDPGM		0xAD
#define DATAFLASH_CMD_RDSR				0x05
#define DATAFLASH_CMD_EWSR				0x50
#define DATAFLASH_CMD_WRSR				0x01
#define DATAFLASH_CMD_WREN				0x06
#define DATAFLASH_CMD_WRDI				0x04
#define DATAFLASH_CMD_RDID				0x90
#define DATAFLASH_CMD_JEDECID			0x9F
#define DATAFLASH_CMD_EBSY				0x70
#define DATAFLASH_CMD_DBSY				0x80

static void _SPIFlashWRSR(char data);
static char _SPIFlashBusy(void);
static void _SPIFlashAddr(DWORD addr);
static void _SPIFlashWREN(void);
static void _SPIFlashEWSR(void);
static void _SPIFlashWRDI(void);
static void _SPIFlashDisable(void);
static void _SPIFlashEnable(void);

// Internal pointer to address being written
static DWORD dwWriteAddr;

void SPIFlashInit()
{
	_SPIFlashWRDI();
	_SPIFlashWRSR(0);
}

void SPIFlashReadArray(DWORD addr, BYTE *dest, WORD len)
{
	while(_SPIFlashBusy());
	_SPIFlashEnable();
	WriteSWSPI( DATAFLASH_CMD_READ );
	_SPIFlashAddr( addr );
	while (len--) *dest++ = WriteSWSPI(0);
	_SPIFlashDisable();
}

void SPIFlashBeginWrite(DWORD dwAddr)
{
    dwWriteAddr = dwAddr;
}

void SPIFlashWrite(BYTE data)
{
	while(_SPIFlashBusy());
	_SPIFlashWREN();
	_SPIFlashEnable();
	WriteSWSPI( DATAFLASH_CMD_BYTEPGM );
	_SPIFlashAddr( dwWriteAddr );
	WriteSWSPI(data);
	_SPIFlashDisable();
}

void SPIFlashWriteArray(BYTE *data, WORD len)
{
	//Outer loop to do sector erase and reinitialize multi-byte writing
	while (len > 1) {
		unsigned char sendAddr = 1;
		
		// If address is 0 start with chip erase
		if((dwWriteAddr & SPI_FLASH_SECTOR_MASK) == 0)
			SPIFlashEraseSector(dwWriteAddr);

		_SPIFlashWREN();

		while (len > 1) {
			while(_SPIFlashBusy());
			_SPIFlashEnable();
			WriteSWSPI( DATAFLASH_CMD_AAIWORDPGM );
			if (sendAddr) {
				_SPIFlashAddr( dwWriteAddr );
				sendAddr = 0;
			}
			WriteSWSPI(*data++);
			WriteSWSPI(*data++);
			len -= 2;
			dwWriteAddr += 2;
			_SPIFlashDisable();
			while(_SPIFlashBusy());
			
			//Break for sector erase
			if((dwWriteAddr & SPI_FLASH_SECTOR_MASK) == 0)
				break;
		}
		_SPIFlashWRDI();
	}
}

void SPIFlashEraseSector(DWORD addr)
{
	while(_SPIFlashBusy());
	_SPIFlashWREN();
	_SPIFlashEnable();
	WriteSWSPI( DATAFLASH_CMD_4KBERASE );
	_SPIFlashAddr( addr );
	_SPIFlashDisable();
}

char _SPIFlashBusy(void) { return (SPIFlashRDSR() & 1); }

char SPIFlashRDSR(void)
{
	char data;
	_SPIFlashEnable();
	WriteSWSPI( DATAFLASH_CMD_RDSR );
	data = WriteSWSPI(0);
	_SPIFlashDisable();
	return data;
}

void _SPIFlashWRSR(char data)
{
	while(_SPIFlashBusy());
	_SPIFlashEWSR();
	_SPIFlashEnable();
	WriteSWSPI( DATAFLASH_CMD_WRSR );
	WriteSWSPI(data);
	_SPIFlashDisable();
}

void _SPIFlashAddr(DWORD addr)
{
		WriteSWSPI( addr >> 16u );
		WriteSWSPI( (addr >> 8u) & 255u );
		WriteSWSPI( addr & 255u );
}

void _SPIFlashWREN() {
	while(_SPIFlashBusy());
	_SPIFlashEnable();
	WriteSWSPI( DATAFLASH_CMD_WREN );
	_SPIFlashDisable();
}

void _SPIFlashEWSR() {
	_SPIFlashEnable();
	WriteSWSPI( DATAFLASH_CMD_EWSR );
	_SPIFlashDisable();
}

void _SPIFlashWRDI() {
	_SPIFlashEnable();
	WriteSWSPI( DATAFLASH_CMD_WRDI );
	_SPIFlashDisable();
}

void _SPIFlashDisable() {
	Nop(); Nop();
	SW_CS2_PIN = 1;
	Nop(); Nop();
}

void _SPIFlashEnable() {
	Nop(); Nop();
	SW_CS2_PIN = 0;
	Nop(); Nop();
}
