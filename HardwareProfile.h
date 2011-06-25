/*********************************************************************
 *
 *	Hardware specific definitions for:
 *    - PICDEM.net 2
 *    - PIC18F97J60
 *    - Internal 10BaseT Ethernet
 *
 *********************************************************************
 * FileName:        HardwareProfile.h
 * Dependencies:    Compiler.h
 * Processor:       PIC18
 * Compiler:        Microchip C18 v3.36 or higher
 * Company:         Microchip Technology, Inc.
 *
 * Software License Agreement
 *
 * Copyright (C) 2002-2010 Microchip Technology Inc.  All rights
 * reserved.
 *
 * Microchip licenses to you the right to use, modify, copy, and
 * distribute:
 * (i)  the Software when embedded on a Microchip microcontroller or
 *      digital signal controller product ("Device") which is
 *      integrated into Licensee's product; or
 * (ii) ONLY the Software driver source files ENC28J60.c, ENC28J60.h,
 *		ENCX24J600.c and ENCX24J600.h ported to a non-Microchip device
 *		used in conjunction with a Microchip ethernet controller for
 *		the sole purpose of interfacing with the ethernet controller.
 *
 * You should refer to the license agreement accompanying this
 * Software for additional information regarding your rights and
 * obligations.
 *
 * THE SOFTWARE AND DOCUMENTATION ARE PROVIDED "AS IS" WITHOUT
 * WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 * LIMITATION, ANY WARRANTY OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * MICROCHIP BE LIABLE FOR ANY INCIDENTAL, SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF
 * PROCUREMENT OF SUBSTITUTE GOODS, TECHNOLOGY OR SERVICES, ANY CLAIMS
 * BY THIRD PARTIES (INCLUDING BUT NOT LIMITED TO ANY DEFENSE
 * THEREOF), ANY CLAIMS FOR INDEMNITY OR CONTRIBUTION, OR OTHER
 * SIMILAR COSTS, WHETHER ASSERTED ON THE BASIS OF CONTRACT, TORT
 * (INCLUDING NEGLIGENCE), BREACH OF WARRANTY, OR OTHERWISE.
 *
 *
 * Author               Date		Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Howard Schlunder		09/16/2010	Regenerated for specific boards
 ********************************************************************/
#ifndef HARDWARE_PROFILE_H
#define HARDWARE_PROFILE_H

#include "Compiler.h"

// Define a macro describing this hardware set up (used in other files)
//#define PICDEMNET2


// Clock frequency values
// These directly influence timed events using the Tick module.  They also are used for UART and SPI baud rate generation.
#define GetSystemClock()		(41666667ul)			// Hz
#define GetInstructionClock()	(GetSystemClock()/4)	// Normally GetSystemClock()/4 for PIC18, GetSystemClock()/2 for PIC24/dsPIC, and GetSystemClock()/1 for PIC32.  Might need changing if using Doze modes.
#define GetPeripheralClock()	(GetSystemClock()/4)	// Normally GetSystemClock()/4 for PIC18, GetSystemClock()/2 for PIC24/dsPIC, and GetSystemClock()/1 for PIC32.  Divisor may be different if using a PIC32 since it's configurable.


// Hardware I/O pin mappings

// I/O pins
#define LED0_TRIS			(PRODL)	/* NC */
#define LED0_IO				(PRODL)	/* NC */
#define LED1_TRIS			(TRISBbits.TRISB4)
#define LED1_IO				(LATBbits.LATB4)
#define LED2_TRIS			(TRISEbits.TRISE2)
#define LED2_IO				(LATEbits.LATE2)


/* ignore rest of LEDs */
#define LED3_TRIS			(PRODL)
#define LED3_IO				(PRODL)
#define LED4_TRIS			(PRODL)
#define LED4_IO				(PRODL)
#define LED5_TRIS			(PRODL)
#define LED5_IO				(PRODL)
#define LED6_TRIS			(PRODL)
#define LED6_IO				(PRODL)
#define LED7_TRIS			(PRODL)
#define LED7_IO				(PRODL)
#define LED_GET()			(LED1_IO)
#define LED_PUT(a)			(LED1_IO = (a))

#define BUTTON0_TRIS		(PRODL)
#define	BUTTON0_IO			(PRODL)
#define BUTTON1_TRIS		(PRODL)
#define	BUTTON1_IO			(PRODL)
#define BUTTON2_TRIS		(PRODL)
#define	BUTTON2_IO			(PRODL)
#define BUTTON3_TRIS		(PRODL)
#define	BUTTON3_IO			(PRODL)

//I2C I/O Pins
#define I2C_SCL_TRIS		(TRISCbits.TRISC3)
#define I2C_SDA_TRIS		(TRISCbits.TRISC4)
#define I2C_SSPBUF			(SSP1BUF)
#define I2C_SPISTAT			(SSP1STAT)
#define I2C_SPISTATbits		(SSP1STATbits)
#define I2C_SPICON1			(SSP1CON1)
#define I2C_SPICON1bits		(SSP1CON1bits)
#define I2C_SPICON2			(SSP1CON2)

// ENC28J60 I/O pins
//#define ENC_RST_TRIS		(TRISDbits.TRISD2)	// Not connected by default
//#define ENC_RST_IO			(LATDbits.LATD2)
//define ENC_CS_TRIS			(TRISDbits.TRISD3)	// Uncomment this line if you wish to use the ENC28J60 on the PICDEM.net 2 board instead of the internal PIC18F97J60 Ethernet module
//#define ENC_CS_IO			(LATDbits.LATD3)
//#define ENC_SCK_TRIS		(TRISCbits.TRISC3)
//#define ENC_SDI_TRIS		(TRISCbits.TRISC4)
//#define ENC_SDO_TRIS		(TRISCbits.TRISC5)
//#define ENC_SPI_IF			(PIR1bits.SSPIF)
//#define ENC_SSPBUF			(SSP1BUF)
//#define ENC_SPISTAT			(SSP1STAT)
//#define ENC_SPISTATbits		(SSP1STATbits)
//#define ENC_SPICON1			(SSP1CON1)
//#define ENC_SPICON1bits		(SSP1CON1bits)
//#define ENC_SPICON2			(SSP1CON2)

/*
#define EEPROM_CS_TRIS		(PRODL)
#define EEPROM_CS_IO		(PRODL)
#define EEPROM_SCK_TRIS		(PRODL)
#define EEPROM_SDI_TRIS		(PRODL)
#define EEPROM_SDO_TRIS		(PRODL)
#define EEPROM_SPI_IF		(PRODL)
#define EEPROM_SSPBUF		(PRODL)
#define EEPROM_SPICON1		(PRODL)
#define EEPROM_SPICON1bits	(PRODL)
#define EEPROM_SPICON2		(PRODL)
#define EEPROM_SPISTAT		(PRODL)
#define EEPROM_SPISTATbits	(PRODL)
*/

/*
#define GetSystemClock()		(41666667ul)      // Hz
#define GetInstructionClock()	(GetSystemClock()/4)
#define GetPeripheralClock()	GetInstructionClock()
*/

#define BusyUART()				BusyUSART()
#define CloseUART()				CloseUSART()
#define ConfigIntUART(a)		ConfigIntUSART(a)
#define DataRdyUART()			DataRdyUSART()
#define OpenUART(a,b,c)			OpenUSART(a,b,c)
#define ReadUART()				ReadUSART()
#define WriteUART(a)			WriteUSART(a)
#define getsUART(a,b,c)			getsUSART(b,a)
#define putsUART(a)				putsUSART(a)
#define getcUART()				ReadUSART()
#define putcUART(a)				WriteUSART(a)
#define putrsUART(a)			putrsUSART((far rom char*)a)

#endif // #ifndef HARDWARE_PROFILE_H
