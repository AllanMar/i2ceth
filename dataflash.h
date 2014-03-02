#ifndef DATAFLASH_COMM
#define DATAFLASH_COMM

#include "HardwareProfile.h"
#include "sw_spi.h"
#include <delays.h>
#include "GenericTypeDefs.h"

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

void SPIFlashReadBytes(unsigned short long addr, char *dest, unsigned char len);
char SPIFlashReadByte(unsigned short long addr);
void SPIFlashWriteByte(char data);
void SPIFlashWriteBytes(char *dest, unsigned char len);
void SPIFlashEraseSector(unsigned short long addr);
char SPIFlashRDSR(void);
void SPIFlashWRSR(char data);
char SPIFlashBusy(void);
void SPIFlashAddr(unsigned short long addr);
void SPIFlashWREN(void);
void SPIFlashDisable(void);
void SPIFlashEnable(void);

#endif