#include "eeprom.h"

void eepromReadBytes(char *dest, unsigned char addr, unsigned char len)
{
	char *pDest = dest;
	eepromEnable();
	WriteSWSPI( EEPROM_CMD_READ );
	WriteSWSPI( addr );

	while (len--) *pDest++ = ReadSWSPI();
	eepromDisable();
}

char eepromReadByte(unsigned char addr)
{
	char data;
	eepromEnable();
	WriteSWSPI( EEPROM_CMD_READ );
	WriteSWSPI( addr );
	data = ReadSWSPI();
	eepromDisable();
	return data;
}

//Write array of bytes to EEPROM committing after a page limit is reached (xxxx0000)
void eepromWriteBytes(char *data, unsigned char addr, unsigned char len)
{
	char *pData = data;
	while (len) {
		eepromWREN();
		eepromEnable();
		WriteSWSPI( EEPROM_CMD_WRITE );
		WriteSWSPI( addr );
		while (len) {
			WriteSWSPI(*pData++); len--; addr++;
			if (!(addr & 15)) break; //Next byte is a new page so save last page
		}
		eepromDisable();
		Delay1KTCYx(65);
	}	
}

void eepromWriteByte(char data, unsigned char addr)
{
		eepromWREN();
		eepromEnable();
		WriteSWSPI( EEPROM_CMD_WRITE );
		WriteSWSPI( addr );
		WriteSWSPI(data);
		eepromDisable();
		Delay1KTCYx(65);
}

void eepromClear()
{
	unsigned char pCount, bCount;
	for (pCount = 0u; pCount < 12u; pCount++) {
		eepromWREN();
		eepromEnable();
		WriteSWSPI( EEPROM_CMD_WRITE );
		WriteSWSPI( pCount * 16u );
		for (bCount = 0u; bCount < 16u; bCount++) WriteSWSPI( 0u );
		eepromDisable();
		Delay1KTCYx(65);
	}
}

void eepromWREN() {
	eepromEnable();
	WriteSWSPI( EEPROM_CMD_WREN );
	eepromDisable();
}

void eepromDisable() {
	Nop(); Nop();
	SW_CS_PIN = 1;
	Nop(); Nop();
}

void eepromEnable() {
	Nop(); Nop();
	SW_CS_PIN = 0;
	Nop(); Nop();
}
