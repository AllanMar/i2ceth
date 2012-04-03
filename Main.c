#pragma config WDT = OFF, STVR = ON, XINST = OFF, CP0 = OFF, FOSC = HSPLL, FOSC2 = ON, FCMEN = ON, IESO = ON, WDTPS = 32768, ETHLED = ON

/*
 * This macro uniquely defines this file as the main entry point.
 * There should only be one such definition in the entire project,
 * and this file must define the AppConfig variable as described below.
 */
#define THIS_IS_STACK_APPLICATION

// Include all headers for any enabled TCPIP Stack functions
#include "TCPIP Stack/TCPIP.h"

// Include functions specific to this stack application
#include "Main.h"
#include "BTnic_Comm.h"
#include "sw_spi.h"
#include "eeprom.h"
#include "dataflash.h"
#include "sram.h"
#include <delays.h>

// Declare AppConfig structure and some other supporting stack variables
APP_CONFIG AppConfig;
static unsigned short wOriginalAppConfigChecksum;	// Checksum of the ROM defaults for AppConfig

// Private helper functions.
// These may or may not be present in all applications.
static void InitAppConfig(void);
static void InitializeBoard(void);

//
// PIC18 Interrupt Service Routines
// 
// NOTE: Several PICs, including the PIC18F4620 revision A3 have a RETFIE FAST/MOVFF bug
// The interruptlow keyword is used to work around the bug when using C18


#pragma interruptlow LowISR
void LowISR(void)
{
    TickUpdate();
}


#pragma interruptlow HighISR
void HighISR(void)
{
	if (PIR1bits.SSP1IF) BTCommRX();
}

#pragma code lowVector=0x18
void LowVector(void){_asm goto LowISR _endasm}

#pragma code highVector=0x8
void HighVector(void){_asm goto HighISR _endasm}


#pragma code // Return to default code section


void main(void)
{
	static DWORD t = 0;

	// Initialize application specific hardware
	InitializeBoard();

	// Initialize stack-related hardware components that may be 
	// required by the UART configuration routines
    TickInit();

	#if defined(STACK_USE_MPFS) || defined(STACK_USE_MPFS2)
	MPFSInit();
	#endif

    if(BUTTON0_IO == 0u)
    {
		// Invalidate the EEPROM contents if BUTTON0 is held down for more than 4 seconds
		DWORD StartTime = TickGet();
		LED1_IO = LED2_IO = 1;
				
		while(BUTTON0_IO == 0u)
		{
			if(TickGet() - StartTime > 4*TICK_SECOND)
			{
				//Clear EEPROM
				eepromClear();
				LED1_IO = LED2_IO = 0;
				while(BUTTON0_IO == 0u);
				break;
			}
		}
    }

	// Initialize Stack and application related NV variables into AppConfig.
	InitAppConfig();

	// Initialize core stack layers (MAC, ARP, TCP, UDP) and
	// application modules (HTTP, SNMP, etc.)
    StackInit();
	BTCommInit();

    while(1)
    {
        if(TickGet() - t >= TICK_SECOND/4ul)
        {
            t = TickGet();
            LED1_IO ^= 1;
            LED2_IO ^= 1;
        }

		if(BUTTON0_IO == 0u) {
			Delay10KTCYx(0);
			while (BUTTON0_IO == 0u);
			RCONbits.POR = 0;
			Reset();
		}

        // This task performs normal stack task including checking
        // for incoming packet, type of packet and calling
        // appropriate stack entity to process it.
        StackTask();

        // This tasks invokes each of the core stack application tasks
        StackApplications();
	}
}

static void InitializeBoard(void)
{	
	// LEDs
	LED0_TRIS = 0;
	LED1_TRIS = 0;
	LED2_TRIS = 0;
	LED3_TRIS = 0;
	LED4_TRIS = 0;
	LED5_TRIS = 0;
	LED6_TRIS = 0;
	LED7_TRIS = 0;
	LED_PUT(0x00);

	CMCON = 0x00; //Turn off analogs inputs and make all digital
	ADCON1 = 0x0F;
	BUTTON0_TRIS = 1;

	// Enable 4x/5x/96MHz PLL on PIC18F87J10, PIC18F97J60, PIC18F87J50, etc.
    OSCTUNE = 0x40;

	// Configure USART
    TXSTA = 0x20;
    RCSTA = 0x90;

	// Enable Interrupts
	RCONbits.IPEN = 1;		// Enable interrupt priorities
    INTCONbits.GIEH = 1;
    INTCONbits.GIEL = 1;

	// configure software SPI
	TRIS_SW_CS_PIN = 0;		// Make the EEPROM CS pin an output
	TRIS_SW_CS2_PIN = 0;	// Make the DATAFLASH CS pin an output
	TRIS_SW_CS3_PIN = 0;	// Make the SRAM CS pin an output
	SW_CS_PIN = 1;			// Pull EEPROM CS HIGH
	SW_CS2_PIN = 1;			// Pull DATAFLASH CS HIGH
	SW_CS3_PIN = 1;			// Pull SRAM CS HIGH
	OpenSWSPI();
}

static void InitAppConfig(void)
{
	unsigned char vNeedToSaveDefaults = 0;
	while (1)
	{
		NVM_VALIDATION_STRUCT NVMValidationStruct;
		memset((void*)&AppConfig, 0x00, sizeof(AppConfig));
		
		AppConfig.Flags.bIsDHCPEnabled = TRUE;
		AppConfig.Flags.bInConfigMode = TRUE;
	
		eepromReadBytes((void*)&AppConfig.MyMACAddr, EEPROM_MAP_MACADDR, sizeof(AppConfig.MyMACAddr));
	
		AppConfig.MyIPAddr.Val = MY_DEFAULT_IP_ADDR_BYTE1 | MY_DEFAULT_IP_ADDR_BYTE2<<8ul | MY_DEFAULT_IP_ADDR_BYTE3<<16ul | MY_DEFAULT_IP_ADDR_BYTE4<<24ul;
		AppConfig.DefaultIPAddr.Val = AppConfig.MyIPAddr.Val;
		AppConfig.MyMask.Val = MY_DEFAULT_MASK_BYTE1 | MY_DEFAULT_MASK_BYTE2<<8ul | MY_DEFAULT_MASK_BYTE3<<16ul | MY_DEFAULT_MASK_BYTE4<<24ul;
		AppConfig.DefaultMask.Val = AppConfig.MyMask.Val;
		AppConfig.MyGateway.Val = MY_DEFAULT_GATE_BYTE1 | MY_DEFAULT_GATE_BYTE2<<8ul | MY_DEFAULT_GATE_BYTE3<<16ul | MY_DEFAULT_GATE_BYTE4<<24ul;
		AppConfig.PrimaryDNSServer.Val = MY_DEFAULT_PRIMARY_DNS_BYTE1 | MY_DEFAULT_PRIMARY_DNS_BYTE2<<8ul  | MY_DEFAULT_PRIMARY_DNS_BYTE3<<16ul  | MY_DEFAULT_PRIMARY_DNS_BYTE4<<24ul;
		AppConfig.SecondaryDNSServer.Val = MY_DEFAULT_SECONDARY_DNS_BYTE1 | MY_DEFAULT_SECONDARY_DNS_BYTE2<<8ul  | MY_DEFAULT_SECONDARY_DNS_BYTE3<<16ul  | MY_DEFAULT_SECONDARY_DNS_BYTE4<<24ul;
			
		// Load the default NetBIOS Host Name
		memcpypgm2ram(AppConfig.NetBIOSName, (ROM void*)MY_DEFAULT_HOST_NAME, 16);
		FormatNetBIOSName(AppConfig.NetBIOSName);

		// Compute the checksum of the AppConfig defaults as loaded from ROM
		wOriginalAppConfigChecksum = CalcIPChecksum((BYTE*)&AppConfig, sizeof(AppConfig));

		// Check to see if we have a flag set indicating that we need to 
		// save the ROM default AppConfig values.
		if(vNeedToSaveDefaults) SaveAppConfig(&AppConfig);
	
		// Read the NVMValidation record and AppConfig struct out of EEPROM/Flash

		eepromReadBytes((void*)&NVMValidationStruct, EEPROM_MAP_NVMVALID, sizeof(NVMValidationStruct));
		eepromReadBytes((void*)&AppConfig, EEPROM_MAP_APPCONFIG, sizeof(AppConfig));

		// Check EEPROM/Flash validitity.  If it isn't valid, set a flag so 
		// that we will save the ROM default values on the next loop 
		// iteration.
		if((NVMValidationStruct.wConfigurationLength != sizeof(AppConfig)) ||
		   (NVMValidationStruct.wOriginalChecksum != wOriginalAppConfigChecksum) ||
		   (NVMValidationStruct.wCurrentChecksum != CalcIPChecksum((BYTE*)&AppConfig, sizeof(AppConfig))))
		{
			// Check to ensure that the vNeedToSaveDefaults flag is zero, 
			// indicating that this is the first iteration through the do 
			// loop.  If we have already saved the defaults once and the 
			// EEPROM/Flash still doesn't pass the validity check, then it 
			// means we aren't successfully reading or writing to the 
			// EEPROM/Flash.  This means you have a hardware error and/or 
			// SPI configuration error.
			if(vNeedToSaveDefaults) while(1);
			
			// Set flag and restart loop to load ROM defaults and save them
			vNeedToSaveDefaults = 1;
			continue;
		}
		
		// If we get down here, it means the EEPROM/Flash has valid contents 
		// and either matches the ROM defaults or previously matched and 
		// was run-time reconfigured by the user.  In this case, we shall 
		// use the contents loaded from EEPROM/Flash.
		break;

	}
}


void SaveAppConfig(const APP_CONFIG *ptrAppConfig)
{
	char count;
	NVM_VALIDATION_STRUCT NVMValidationStruct;

	// Get proper values for the validation structure indicating that we can use 
	// these EEPROM/Flash contents on future boot ups
	NVMValidationStruct.wOriginalChecksum = wOriginalAppConfigChecksum;
	NVMValidationStruct.wCurrentChecksum = CalcIPChecksum((BYTE*)ptrAppConfig, sizeof(APP_CONFIG));
	NVMValidationStruct.wConfigurationLength = sizeof(APP_CONFIG);

	// Write the validation struct and current AppConfig contents to EEPROM/Flash
	eepromWriteBytes((void*)&NVMValidationStruct, EEPROM_MAP_NVMVALID, sizeof(NVMValidationStruct));
	eepromWriteBytes((void*)ptrAppConfig, EEPROM_MAP_APPCONFIG, sizeof(APP_CONFIG));
}