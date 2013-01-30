#ifndef _MAIN_H
#define _MAIN_H

#include "TCPIP Stack/StackTsk.h"

// Define a header structure for validating the AppConfig data structure in EEPROM/Flash
typedef struct
{
	unsigned short wConfigurationLength;	// Number of bytes saved in EEPROM/Flash (sizeof(APP_CONFIG))
	unsigned short wOriginalChecksum;		// Checksum of the original AppConfig defaults as loaded from ROM (to detect when to wipe the EEPROM/Flash record of AppConfig due to a stack change, such as when switching from Ethernet to Wi-Fi)
	unsigned short wCurrentChecksum;		// Checksum of the current EEPROM/Flash data.  This protects against using corrupt values if power failure occurs while writing them and helps detect coding errors in which some other task writes to the EEPROM in the AppConfig area.
} NVM_VALIDATION_STRUCT;

void SaveAppConfig(const APP_CONFIG*);

typedef struct
{
	unsigned int	HTTPPort;
	unsigned int	HTTPSPort;
	BYTE	AuthUser[16];
	BYTE	AuthPwd[16];
	struct
	{
		unsigned char : 6;
		unsigned char DataRequireHTTPS : 1;
		unsigned char DataRequireAuth : 1;
	} Flags;
} WEBSRV_CONFIG;

#define WEBSRV_DEFAULTUSER	"admin"
#define WEBSRV_DEFAULTPWD	"password"
#define WEBSRV_DEFAULTHTTP	80
#define WEBSRV_DEFAULTHTTPS	443

void SaveWebSrvConfig(const WEBSRV_CONFIG*);

extern WEBSRV_CONFIG WebSrvConfig;

#endif // _MAIN_H
