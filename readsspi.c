#include "sw_spi.h"

/********************************************************************
*       Function Name:  WriteSWSPI                                  *
*       Return Value:   char: received data                         *
*       Parameters:     data: data to transmit                      *
*       Description:    This routine sets the CS pin high.          *
********************************************************************/
char ReadSWSPI( )
{
	char BitCount;
    unsigned char data;
	unsigned char mask;
	data = 0x00;
	BitCount = 8;
	mask = 0x80;


        do                              // Loop 8 times
        {
                Nop();                  // Produces a 50% duty cycle clock
                Nop();
                Nop();
                Nop();
                Nop();
                Nop();
                Nop();
                Nop();
                Nop();
                Nop();
                Nop();
                if(SW_DIN_PIN) data = (data | mask);
                SW_SCK_PIN = 1;         // Set the SCK pin
                Nop();                  // Produces a 50% duty cycle clock
                Nop();
                Nop();
                Nop();
                Nop();
                Nop();
                Nop();
                Nop();
                Nop();
                Nop();
                Nop();
                SW_SCK_PIN = 0;         // Clear the SCK pin
                BitCount--;             // Count iterations through loop
				mask = mask>>1;
        } while(BitCount);
	return data;
}