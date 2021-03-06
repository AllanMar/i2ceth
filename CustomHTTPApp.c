/*********************************************************************
 *
 *  Application to Demo HTTP2 Server
 *  Support for HTTP2 module in Microchip TCP/IP Stack
 *	 -Implements the application 
 *	 -Reference: RFC 1002
 *
 *********************************************************************
 * FileName:        CustomHTTPApp.c
 * Dependencies:    TCP/IP stack
 * Processor:       PIC18, PIC24F, PIC24H, dsPIC30F, dsPIC33F, PIC32
 * Compiler:        Microchip C32 v1.05 or higher
 *					Microchip C30 v3.12 or higher
 *					Microchip C18 v3.30 or higher
 *					HI-TECH PICC-18 PRO 9.63PL2 or higher
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
 * Author               Date    Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Elliott Wood     	6/18/07	Original
 ********************************************************************/
#define __CUSTOMHTTPAPP_C

#include "TCPIPConfig.h"
#include "BTnic_Comm.h"
#include "eeprom.h"
#include "sram.h"

#if defined(STACK_USE_HTTP2_SERVER)

#include "TCPIP Stack/TCPIP.h"
#include "Main.h"		// Needed for SaveAppConfig() prototype

static HTTP_IO_RESULT HTTPPostConfig(void);
static HTTP_IO_RESULT HTTPPostWebConf(void);
static HTTP_IO_RESULT HTTPPostBTCom(void);

// RAM allocated for DDNS parameters
#if defined(STACK_USE_DYNAMICDNS_CLIENT)
	static BYTE DDNSData[100];
#endif

// Sticky status message variable.
// This is used to indicated whether or not the previous POST operation was 
// successful.  The application uses these to store status messages when a 
// POST operation redirects.  This lets the application provide status messages
// after a redirect, when connection instance data has already been lost.
static BOOL lastSuccess = FALSE;

// Stick status message variable.  See lastSuccess for details.
static BOOL lastFailure = FALSE;

/****************************************************************************
  Section:
	Authorization Handlers
  ***************************************************************************/
  
/*****************************************************************************
  Function:
	BYTE HTTPNeedsAuth(BYTE* cFile)
	
  Internal:
  	See documentation in the TCP/IP Stack API or HTTP2.h for details.
  ***************************************************************************/
#if defined(HTTP_USE_AUTHENTICATION)
BYTE HTTPNeedsAuth(BYTE* cFile)
{
	// If the filename begins with the folder "protect", then require auth
	if(memcmppgm2ram(cFile, (ROM void*)"protect", 7) == 0)
		return 0x00;		// Authentication will be needed later

	if(WebSrvConfig.Flags.DataRequireAuth && (!memcmppgm2ram(cFile, "btnic.cgi", 9)))
		return 0x00;		// Authentication will be needed later

	#if defined(HTTP_MPFS_UPLOAD_REQUIRES_AUTH)
	if(memcmppgm2ram(cFile, (ROM void*)"mpfsupload", 10) == 0)
		return 0x00;
	#endif

	// You can match additional strings here to password protect other files.
	// You could switch this and exclude files from authentication.
	// You could also always return 0x00 to require auth for all files.
	// You can return different values (0x00 to 0x79) to track "realms" for below.

	return 0x80;			// No authentication required
}
#endif

/*****************************************************************************
  Function:
	BYTE HTTPCheckAuth(BYTE* cUser, BYTE* cPass)
	
  Internal:
  	See documentation in the TCP/IP Stack API or HTTP2.h for details.
  ***************************************************************************/
#if defined(HTTP_USE_AUTHENTICATION)
BYTE HTTPCheckAuth(BYTE* cUser, BYTE* cPass)
{
	if(strcmp((char *)cUser,WebSrvConfig.AuthUser) == 0
		&& strcmp((char *)cPass, WebSrvConfig.AuthPwd) == 0)
		return 0x80;		// We accept this combination
	
	// You can add additional user/pass combos here.
	// If you return specific "realm" values above, you can base this 
	//   decision on what specific file or folder is being accessed.
	// You could return different values (0x80 to 0xff) to indicate 
	//   various users or groups, and base future processing decisions
	//   in HTTPExecuteGet/Post or HTTPPrint callbacks on this value.
	
	return 0x00;			// Provided user/pass is invalid
}
#endif

/****************************************************************************
  Section:
	GET Form Handlers
  ***************************************************************************/
  
/*****************************************************************************
  Function:
	HTTP_IO_RESULT HTTPExecuteGet(void)
	
  Internal:
  	See documentation in the TCP/IP Stack API or HTTP2.h for details.
  ***************************************************************************/
HTTP_IO_RESULT HTTPExecuteGet(void)
{
	BYTE *ptr;
	BYTE filename[21]; //Single folder (8 char) deep, + 1 x "/" + 8 x filename + 1 x "." + 3 x ext
	int retValue;

	// Load the file name
	// Make sure BYTE filename[] above is large enough for your longest name
	MPFSGetFilename(curHTTP.file, filename, 21);
	

	if(!memcmppgm2ram(filename, "btnic.cgi", 9))
	{
		switch(curHTTP.smPost)
		{
			default: 
				curHTTP.smPost = SM_BTNIC_START;
			case SM_BTNIC_START:
				if (BTCommGetState() != COMMSTATE_IDLE) return HTTP_IO_WAITING;
				if (BTCommRequest(curHTTP.data)) return HTTP_IO_WAITING;
				curHTTP.smPost = SM_BTNIC_WAIT_FOR_RESP;
				
			case SM_BTNIC_WAIT_FOR_RESP:
				switch (BTCommGetState())
				{
					case COMMSTATE_MSG:
						return HTTP_IO_DONE;
					case COMMSTATE_IDLE:
						//BTCommSetRsp("WAIT TIMEOUT");
						return HTTP_IO_DONE;
					default:
						return HTTP_IO_WAITING;
				}
		}
	} 
	return HTTP_IO_DONE;
}


/****************************************************************************
  Section:
	POST Form Handlers
  ***************************************************************************/
#if defined(HTTP_USE_POST)

/*****************************************************************************
  Function:
	HTTP_IO_RESULT HTTPExecutePost(void)
	
  Internal:
  	See documentation in the TCP/IP Stack API or HTTP2.h for details.
  ***************************************************************************/
HTTP_IO_RESULT HTTPExecutePost(void)
{
	// Resolve which function to use and pass along
	BYTE filename[20];
	
	// Load the file name
	// Make sure BYTE filename[] above is large enough for your longest name
	MPFSGetFilename(curHTTP.file, filename, sizeof(filename));
	
#if defined(STACK_USE_HTTP_APP_RECONFIG)
	if(!memcmppgm2ram(filename, "protect/config.htm", 18))
		return HTTPPostConfig();
	if(!memcmppgm2ram(filename, "protect/webconf.htm", 18))
		return HTTPPostWebConf();
#endif
	if(!memcmppgm2ram(filename, "protect/btcom.htm", 17))
		return HTTPPostBTCom();
	return HTTP_IO_DONE;
}

/*****************************************************************************
  Function:
	static HTTP_IO_RESULT HTTPPostConfig(void)

  Summary:
	Processes the configuration form on config/index.htm

  Description:
	Accepts configuration parameters from the form, saves them to a
	temporary location in RAM, then eventually saves the data to EEPROM or
	external Flash.
	
	When complete, this function redirects to config/reboot.htm, which will
	display information on reconnecting to the board.

	This function creates a shadow copy of the AppConfig structure in 
	RAM and then overwrites incoming data there as it arrives.  For each 
	name/value pair, the name is first read to curHTTP.data[0:5].  Next, the 
	value is read to newAppConfig.  Once all data has been read, the new
	AppConfig is saved back to EEPROM and the browser is redirected to 
	reboot.htm.  That file includes an AJAX call to reboot.cgi, which 
	performs the actual reboot of the machine.
	
	If an IP address cannot be parsed, too much data is POSTed, or any other 
	parsing error occurs, the browser reloads config.htm and displays an error 
	message at the top.

  Precondition:
	None

  Parameters:
	None

  Return Values:
  	HTTP_IO_DONE - all parameters have been processed
  	HTTP_IO_NEED_DATA - data needed by this function has not yet arrived
  ***************************************************************************/
#if defined(STACK_USE_HTTP_APP_RECONFIG)
static HTTP_IO_RESULT HTTPPostConfig(void)
{
	APP_CONFIG newAppConfig;
	BYTE *ptr;
	BYTE i;

	// Check to see if the browser is attempting to submit more data than we 
	// can parse at once.  This function needs to receive all updated 
	// parameters and validate them all before committing them to memory so that
	// orphaned configuration parameters do not get written (for example, if a 
	// static IP address is given, but the subnet mask fails parsing, we 
	// should not use the static IP address).  Everything needs to be processed 
	// in a single transaction.  If this is impossible, fail and notify the user.
	// As a web devloper, if you add parameters to AppConfig and run into this 
	// problem, you could fix this by to splitting your update web page into two 
	// seperate web pages (causing two transactional writes).  Alternatively, 
	// you could fix it by storing a static shadow copy of AppConfig someplace 
	// in memory and using it instead of newAppConfig.  Lastly, you could 
	// increase the TCP RX FIFO size for the HTTP server.  This will allow more 
	// data to be POSTed by the web browser before hitting this limit.
	if(curHTTP.byteCount > TCPIsGetReady(sktHTTP) + TCPGetRxFIFOFree(sktHTTP))
		goto ConfigFailure;
	
	// Ensure that all data is waiting to be parsed.  If not, keep waiting for 
	// all of it to arrive.
	if(TCPIsGetReady(sktHTTP) < curHTTP.byteCount)
		return HTTP_IO_NEED_DATA;
	
	
	eepromReadBytes((BYTE*)&newAppConfig, EEPROM_MAP_APPCONFIG, sizeof(newAppConfig));
	
	// Start out assuming that DHCP is disabled.  This is necessary since the 
	// browser doesn't submit this field if it is unchecked (meaning zero).  
	// However, if it is checked, this will be overridden since it will be 
	// submitted.
	newAppConfig.Flags.bIsDHCPEnabled = 0;


	// Read all browser POST data
	while(curHTTP.byteCount)
	{
		// Read a form field name
		if(HTTPReadPostName(curHTTP.data, 6) != HTTP_READ_OK)
			goto ConfigFailure;
			
		// Read a form field value
		if(HTTPReadPostValue(curHTTP.data + 6, sizeof(curHTTP.data)-6-2) != HTTP_READ_OK)
			goto ConfigFailure;
			
		// Parse the value that was read
		if(!strcmppgm2ram((char*)curHTTP.data, (ROM char*)"ip"))
		{// Read new static IP Address
			if(!StringToIPAddress(curHTTP.data+6, &newAppConfig.MyIPAddr))
				goto ConfigFailure;
				
			newAppConfig.DefaultIPAddr.Val = newAppConfig.MyIPAddr.Val;
		}
		else if(!strcmppgm2ram((char*)curHTTP.data, (ROM char*)"gw"))
		{// Read new gateway address
			if(!StringToIPAddress(curHTTP.data+6, &newAppConfig.MyGateway))
				goto ConfigFailure;
		}
		else if(!strcmppgm2ram((char*)curHTTP.data, (ROM char*)"sub"))
		{// Read new static subnet
			if(!StringToIPAddress(curHTTP.data+6, &newAppConfig.MyMask))
				goto ConfigFailure;

			newAppConfig.DefaultMask.Val = newAppConfig.MyMask.Val;
		}
		else if(!strcmppgm2ram((char*)curHTTP.data, (ROM char*)"dns1"))
		{// Read new primary DNS server
			if(!StringToIPAddress(curHTTP.data+6, &newAppConfig.PrimaryDNSServer))
				goto ConfigFailure;
		}
		else if(!strcmppgm2ram((char*)curHTTP.data, (ROM char*)"dns2"))
		{// Read new secondary DNS server
			if(!StringToIPAddress(curHTTP.data+6, &newAppConfig.SecondaryDNSServer))
				goto ConfigFailure;
		}
		else if(!strcmppgm2ram((char*)curHTTP.data, (ROM char*)"mac"))
		{
			// Read new MAC address
			WORD w;
			BYTE i;

			ptr = curHTTP.data+6;

			for(i = 0; i < 12u; i++)
			{// Read the MAC address
				
				// Skip non-hex bytes
				while( *ptr != 0x00u && !(*ptr >= '0' && *ptr <= '9') && !(*ptr >= 'A' && *ptr <= 'F') && !(*ptr >= 'a' && *ptr <= 'f') )
					ptr++;

				// MAC string is over, so zeroize the rest
				if(*ptr == 0x00u)
				{
					for(; i < 12u; i++)
						curHTTP.data[i] = '0';
					break;
				}
				
				// Save the MAC byte
				curHTTP.data[i] = *ptr++;
			}
			
			// Read MAC Address, one byte at a time
			for(i = 0; i < 6u; i++)
			{
				((BYTE*)&w)[1] = curHTTP.data[i*2];
				((BYTE*)&w)[0] = curHTTP.data[i*2+1];
				newAppConfig.MyMACAddr.v[i] = hexatob(*((WORD_VAL*)&w));
			}
		}
		else if(!strcmppgm2ram((char*)curHTTP.data, (ROM char*)"host"))
		{// Read new hostname
			FormatNetBIOSName(&curHTTP.data[6]);
			memcpy((void*)newAppConfig.NetBIOSName, (void*)curHTTP.data+6, 16);
		}
		else if(!strcmppgm2ram((char*)curHTTP.data, (ROM char*)"dhcp"))
		{// Read new DHCP Enabled flag
			if(curHTTP.data[6] == '1')
				newAppConfig.Flags.bIsDHCPEnabled = 1;
		}
	}


	// All parsing complete!  Save new settings and force a reboot
	SaveAppConfig(&newAppConfig);
	
	// Set the board to reboot and display reconnecting information
	strcpypgm2ram((char*)curHTTP.data, "/protect/reboot.htm?");
	memcpy((void*)(curHTTP.data+20), (void*)newAppConfig.NetBIOSName, 16);
	curHTTP.data[20+16] = 0x00;	// Force null termination
	for(i = 20; i < 20u+16u; i++)
	{
		if(curHTTP.data[i] == ' ')
			curHTTP.data[i] = 0x00;
	}		
	curHTTP.httpStatus = HTTP_REDIRECT;	
	
	return HTTP_IO_DONE;

ConfigFailure:
	lastFailure = TRUE;
	strcpypgm2ram((char*)curHTTP.data, "/protect/config.htm");
	curHTTP.httpStatus = HTTP_REDIRECT;		

	return HTTP_IO_DONE;
}

static HTTP_IO_RESULT HTTPPostWebConf(void)
{

	WEBSRV_CONFIG newWebSrvConfig;
	BYTE *ptr;
	BYTE i;

	if(curHTTP.byteCount > TCPIsGetReady(sktHTTP) + TCPGetRxFIFOFree(sktHTTP))
		goto WebConfigFailure;
	
	// Ensure that all data is waiting to be parsed.  If not, keep waiting for 
	// all of it to arrive.
	if(TCPIsGetReady(sktHTTP) < curHTTP.byteCount)
		return HTTP_IO_NEED_DATA;
	
	eepromReadBytes((BYTE*)&newWebSrvConfig, EEPROM_MAP_WEBSRVCONFIG, sizeof(newWebSrvConfig));
	
	// Start out assuming flags are disabled.  This is necessary since the 
	// browser doesn't submit this field if it is unchecked (meaning zero).  
	// However, if it is checked, this will be overridden since it will be 
	// submitted.
	newWebSrvConfig.Flags.DataRequireHTTPS = 0;
	newWebSrvConfig.Flags.DataRequireAuth = 0;

	// Read all browser POST data
	while(curHTTP.byteCount)
	{
		// Read a form field name
		if(HTTPReadPostName(curHTTP.data, 6) != HTTP_READ_OK)
			goto WebConfigFailure;
			
		// Read a form field value
		if(HTTPReadPostValue(curHTTP.data + 6, sizeof(curHTTP.data)-6-2) != HTTP_READ_OK)
			goto WebConfigFailure;
			
		// Parse the value that was read
		if(!strcmppgm2ram((char*)curHTTP.data, (ROM char*)"http"))
		{
			newWebSrvConfig.HTTPPort = atol(curHTTP.data + 6);
		}
		else if(!strcmppgm2ram((char*)curHTTP.data, (ROM char*)"ssl"))
		{
			newWebSrvConfig.HTTPSPort = atol(curHTTP.data + 6);
		}
		else if(!strcmppgm2ram((char*)curHTTP.data, (ROM char*)"user"))
		{
			strcpy((void*)newWebSrvConfig.AuthUser, (void*)curHTTP.data+6);
		}
		else if(!strcmppgm2ram((char*)curHTTP.data, (ROM char*)"pass"))
		{
			strcpy((void*)newWebSrvConfig.AuthPwd, (void*)curHTTP.data+6);
		}
		else if(!strcmppgm2ram((char*)curHTTP.data, (ROM char*)"reqS"))
		{
			if(curHTTP.data[6] == '1')
				newWebSrvConfig.Flags.DataRequireHTTPS = 1;
		}
		else if(!strcmppgm2ram((char*)curHTTP.data, (ROM char*)"reqA"))
		{
			if(curHTTP.data[6] == '1')
				newWebSrvConfig.Flags.DataRequireAuth = 1;
		}
//Timeout on Idle is not supported
/*		else if(!strcmppgm2ram((char*)curHTTP.data, (ROM char*)"idle"))
		{
			newWebSrvConfig.StateTimeout[COMMSTATE_IDLE] = atol(curHTTP.data + 6);
		} */
		else if(!strcmppgm2ram((char*)curHTTP.data, (ROM char*)"buff"))
		{
			newWebSrvConfig.StateTimeout[COMMSTATE_BUFFERING] = atol(curHTTP.data + 6);
		}
		else if(!strcmppgm2ram((char*)curHTTP.data, (ROM char*)"txr"))
		{
			newWebSrvConfig.StateTimeout[COMMSTATE_TXREADY] = atol(curHTTP.data + 6);
		}
		else if(!strcmppgm2ram((char*)curHTTP.data, (ROM char*)"tx"))
		{
			newWebSrvConfig.StateTimeout[COMMSTATE_TX] = atol(curHTTP.data + 6);
		}
		else if(!strcmppgm2ram((char*)curHTTP.data, (ROM char*)"wait"))
		{
			newWebSrvConfig.StateTimeout[COMMSTATE_WAIT] = atol(curHTTP.data + 6);
		}
		else if(!strcmppgm2ram((char*)curHTTP.data, (ROM char*)"rx"))
		{
			newWebSrvConfig.StateTimeout[COMMSTATE_RX] = atol(curHTTP.data + 6);
		}
		else if(!strcmppgm2ram((char*)curHTTP.data, (ROM char*)"msg"))
		{
			newWebSrvConfig.StateTimeout[COMMSTATE_MSG] = atol(curHTTP.data + 6);
		}
	}


	// All parsing complete!  Save new settings and force a reboot
	SaveWebSrvConfig(&newWebSrvConfig);
	
	// Set the board to reboot and display reconnecting information
	strcpypgm2ram((char*)curHTTP.data, "/protect/reboot.htm?");
	curHTTP.data[20] = 0x00;	// Force null termination
	for(i = 20; i < 20u+16u; i++)
	curHTTP.httpStatus = HTTP_REDIRECT;	
	
	return HTTP_IO_DONE;

WebConfigFailure:
	lastFailure = TRUE;
	strcpypgm2ram((char*)curHTTP.data, "/protect/webconf.htm");
	curHTTP.httpStatus = HTTP_REDIRECT;		

	return HTTP_IO_DONE;
}

static HTTP_IO_RESULT HTTPPostBTCom(void)
{

	BYTE *ptr;
	BYTE i;

	// Ensure that all data is waiting to be parsed.  If not, keep waiting for 
	// all of it to arrive.
	if(TCPIsGetReady(sktHTTP) < curHTTP.byteCount)
		return HTTP_IO_NEED_DATA;
	
	// Read all browser POST data
	while(curHTTP.byteCount)
	{
		// Read a form field name
		HTTPReadPostName(curHTTP.data, 6);
		HTTPReadPostValue(curHTTP.data + 6, sizeof(curHTTP.data)-6-2);
			
		// Parse the value that was read
		if(!strcmppgm2ram((char*)curHTTP.data, (ROM char*)"rst"))
		{
			BTCommInit();
		}
	}
	return HTTP_IO_DONE;
}


#endif //(use_post)

#endif

void HTTPPrint_BTVer(void)
{
	TCPPutROMString(sktHTTP, (ROM void*)"2.5 Build 1");
}

void HTTPPrint_BTState(void)
{
	switch (BTCommGetState())
	{
		case COMMSTATE_IDLE:
			TCPPutROMString(sktHTTP, (ROM void*)"IDLE");
			break;
		case COMMSTATE_BUFFERING:
			TCPPutROMString(sktHTTP, (ROM void*)"BUFFERING");
			break;
		case COMMSTATE_TXREADY:
			TCPPutROMString(sktHTTP, (ROM void*)"TXREADY");
			break;
		case COMMSTATE_TX:
			TCPPutROMString(sktHTTP, (ROM void*)"TX");
			break;
		case COMMSTATE_WAIT:
			TCPPutROMString(sktHTTP, (ROM void*)"WAIT");
			break;
		case COMMSTATE_RX:
			TCPPutROMString(sktHTTP, (ROM void*)"RX");
			break;
		case COMMSTATE_MSG:
			TCPPutROMString(sktHTTP, (ROM void*)"MSG");
			break;
		default:
			TCPPutROMString(sktHTTP, (ROM void*)"UNDEFINED");
			break;
	}
}

void HTTPPrint_BTNic_CGI(void)
{
	WORD len;
	len = TCPIsPutReady(sktHTTP);

	if(curHTTP.callbackPos == 0u) 
	{
		curHTTP.callbackPos = BTCommGetRspLen();
		TCPPutROMString(sktHTTP, (ROM BYTE*)"[\"");
		len -= 2;
	}

	while(len > 5 && curHTTP.callbackPos)
	{
		char byteOut = BTCommGetRsp();
		if (byteOut == '\t') 
		{
			TCPPutROMString(sktHTTP, (ROM BYTE*)"\",\"");
			len -= 3;
		}
		else  len -= TCPPut(sktHTTP, byteOut);
		curHTTP.callbackPos--;
	}
	if(curHTTP.callbackPos == 0u) TCPPutROMString(sktHTTP, (ROM BYTE*)"\"]");
	return;
}

void HTTPPrint_SSP1CON1_WCOL(void)
{
	TCPPut(sktHTTP, SSP1CON1bits.WCOL ? '1' : '0');
}

void HTTPPrint_SSP1CON1_SSPOV(void)
{
	TCPPut(sktHTTP, SSP1CON1bits.SSPOV ? '1' : '0');
}

void HTTPPrint_SSP1CON1_SSPEN(void)
{
	TCPPut(sktHTTP, SSP1CON1bits.SSPEN ? '1' : '0');
}

void HTTPPrint_SSP1CON1_CKP(void)
{
	TCPPut(sktHTTP, SSP1CON1bits.CKP ? '1' : '0');
}

void HTTPPrint_SSP1CON1_SSPM3(void)
{
	TCPPut(sktHTTP, SSP1CON1bits.SSPM3 ? '1' : '0');
}

void HTTPPrint_SSP1CON1_SSPM2(void)
{
	TCPPut(sktHTTP, SSP1CON1bits.SSPM2 ? '1' : '0');
}

void HTTPPrint_SSP1CON1_SSPM1(void)
{
	TCPPut(sktHTTP, SSP1CON1bits.SSPM1 ? '1' : '0');
}

void HTTPPrint_SSP1CON1_SSPM0(void)
{
	TCPPut(sktHTTP, SSP1CON1bits.SSPM0 ? '1' : '0');
}

void HTTPPrint_SSP1CON2_GCEN(void)
{
	TCPPut(sktHTTP, SSP1CON2bits.GCEN ? '1' : '0');
}

void HTTPPrint_SSP1CON2_ACKSTAT(void)
{
	TCPPut(sktHTTP, SSP1CON2bits.ACKSTAT ? '1' : '0');
}

void HTTPPrint_SSP1CON2_ADMSK5(void)
{
	TCPPut(sktHTTP, SSP1CON2bits.ADMSK5 ? '1' : '0');
}

void HTTPPrint_SSP1CON2_ADMSK4(void)
{
	TCPPut(sktHTTP, SSP1CON2bits.ADMSK4 ? '1' : '0');
}

void HTTPPrint_SSP1CON2_ADMSK3(void)
{
	TCPPut(sktHTTP, SSP1CON2bits.ADMSK3 ? '1' : '0');
}

void HTTPPrint_SSP1CON2_ADMSK2(void)
{
	TCPPut(sktHTTP, SSP1CON2bits.ADMSK2 ? '1' : '0');
}

void HTTPPrint_SSP1CON2_ADMSK1(void)
{
	TCPPut(sktHTTP, SSP1CON2bits.ADMSK1 ? '1' : '0');
}

void HTTPPrint_SSP1CON2_SEN(void)
{
	TCPPut(sktHTTP, SSP1CON2bits.SEN ? '1' : '0');
}

void HTTPPrint_SSP1STAT_SMP(void)
{
	TCPPut(sktHTTP, SSP1STATbits.SMP ? '1' : '0');
}

void HTTPPrint_SSP1STAT_CKE(void)
{
	TCPPut(sktHTTP, SSP1STATbits.CKE ? '1' : '0');
}

void HTTPPrint_SSP1STAT_P(void)
{
	TCPPut(sktHTTP, SSP1STATbits.P ? '1' : '0');
}

void HTTPPrint_SSP1STAT_S(void)
{
	TCPPut(sktHTTP, SSP1STATbits.S ? '1' : '0');
}

void HTTPPrint_SSP1STAT_UA(void)
{
	TCPPut(sktHTTP, SSP1STATbits.UA ? '1' : '0');
}

void HTTPPrint_SSP1STAT_BF(void)
{
	TCPPut(sktHTTP, SSP1STATbits.BF ? '1' : '0');
}

void HTTPPrint_SSP1STAT_DA(void)
{
	TCPPut(sktHTTP, SSP1STATbits.D_A ? '1' : '0');
}

void HTTPPrint_SSP1STAT_RW(void)
{
	TCPPut(sktHTTP, SSP1STATbits.R_W ? '1' : '0');
}

void HTTPPrint_BTCommTimer(void)
{
	unsigned char string[20];
	ultoa(BTCommGetTimer() / (TICK_SECOND / 1000), string);
	TCPPutString(sktHTTP, string);
}

void HTTPPrint_TickGet(void)
{
	unsigned char string[20];
	ultoa(TickGet() / (TICK_SECOND / 1000), string);
	TCPPutString(sktHTTP, string);
}

void HTTPPrint_BTResponseLen(void)
{
	unsigned char string[9];
	itoa(BTCommGetRspLen(), string);
	TCPPutString(sktHTTP, string);
}

void HTTPPrint_BTRequestLen(void)
{
	unsigned char string[9];
	itoa(BTCommGetReqLen(), string);
	TCPPutString(sktHTTP, string);
}

void HTTPPrint_BTResponseBuffer(void)
{
	WORD len;
	len = TCPIsPutReady(sktHTTP);

	if(curHTTP.callbackPos == 0u) curHTTP.callbackPos = BTCommGetRspLen();

	while(len && curHTTP.callbackPos)
	{
		len -= TCPPut(sktHTTP, BTCommGetRspBuffer(BTCommGetRspLen() - curHTTP.callbackPos));
		curHTTP.callbackPos--;
	}
	return;
}

void HTTPPrint_BTRequestBuffer(void)
{
	WORD len;
	len = TCPIsPutReady(sktHTTP);

	if(curHTTP.callbackPos == 0u) curHTTP.callbackPos = BTCommGetReqLen();

	while(len && curHTTP.callbackPos)
	{
		len -= TCPPut(sktHTTP, BTCommGetReqBuffer(BTCommGetReqLen() - curHTTP.callbackPos));
		curHTTP.callbackPos--;
	}
	return;
}

extern APP_CONFIG AppConfig;

void HTTPPrintIP(IP_ADDR ip)
{
	BYTE digits[4];
	BYTE i;
	
	for(i = 0; i < 4u; i++)
	{
		if(i)
			TCPPut(sktHTTP, '.');
		uitoa(ip.v[i], digits);
		TCPPutString(sktHTTP, digits);
	}
}

void HTTPPrint_config_hostname(void)
{
	TCPPutString(sktHTTP, AppConfig.NetBIOSName);
	return;
}

void HTTPPrint_config_dhcpchecked(void)
{
	if(AppConfig.Flags.bIsDHCPEnabled)
		TCPPutROMString(sktHTTP, (ROM BYTE*)"checked");
	return;
}

void HTTPPrint_config_ip(void)
{
	HTTPPrintIP(AppConfig.MyIPAddr);
	return;
}

void HTTPPrint_config_gw(void)
{
	HTTPPrintIP(AppConfig.MyGateway);
	return;
}

void HTTPPrint_config_subnet(void)
{
	HTTPPrintIP(AppConfig.MyMask);
	return;
}

void HTTPPrint_config_dns1(void)
{
	HTTPPrintIP(AppConfig.PrimaryDNSServer);
	return;
}

void HTTPPrint_config_dns2(void)
{
	HTTPPrintIP(AppConfig.SecondaryDNSServer);
	return;
}

void HTTPPrint_config_mac(void)
{
	BYTE i;
	
	if(TCPIsPutReady(sktHTTP) < 18u)
	{//need 17 bytes to write a MAC
		curHTTP.callbackPos = 0x01;
		return;
	}	
	
	// Write each byte
	for(i = 0; i < 6u; i++)
	{
		if(i)
			TCPPut(sktHTTP, ':');
		TCPPut(sktHTTP, btohexa_high(AppConfig.MyMACAddr.v[i]));
		TCPPut(sktHTTP, btohexa_low(AppConfig.MyMACAddr.v[i]));
	}
	
	// Indicate that we're done
	curHTTP.callbackPos = 0x00;
	return;
}

void HTTPPrint_config_stateTimeIdle(void)
{
	unsigned char string[20];
	ultoa(WebSrvConfig.StateTimeout[COMMSTATE_IDLE], string);
	TCPPutString(sktHTTP, string);
}

void HTTPPrint_config_stateTimeBuffering(void)
{
	unsigned char string[20];
	ultoa(WebSrvConfig.StateTimeout[COMMSTATE_BUFFERING], string);
	TCPPutString(sktHTTP, string);
}

void HTTPPrint_config_stateTimeTXReady(void)
{
	unsigned char string[20];
	ultoa(WebSrvConfig.StateTimeout[COMMSTATE_TXREADY], string);
	TCPPutString(sktHTTP, string);
}

void HTTPPrint_config_stateTimeTX(void)
{
	unsigned char string[20];
	ultoa(WebSrvConfig.StateTimeout[COMMSTATE_TX], string);
	TCPPutString(sktHTTP, string);
}

void HTTPPrint_config_stateTimeWait(void)
{
	unsigned char string[20];
	ultoa(WebSrvConfig.StateTimeout[COMMSTATE_WAIT], string);
	TCPPutString(sktHTTP, string);
}

void HTTPPrint_config_stateTimeRX(void)
{
	unsigned char string[20];
	ultoa(WebSrvConfig.StateTimeout[COMMSTATE_RX], string);
	TCPPutString(sktHTTP, string);
}

void HTTPPrint_config_stateTimeMsg(void)
{
	unsigned char string[20];
	ultoa(WebSrvConfig.StateTimeout[COMMSTATE_MSG], string);
	TCPPutString(sktHTTP, string);
}


void HTTPPrint_reboot(void)
{
	// This is not so much a print function, but causes the board to reboot
	// when the configuration is changed.  If called via an AJAX call, this
	// will gracefully reset the board and bring it back online immediately
	Reset();
}

void HTTPPrint_rebootaddr(void)
{// This is the expected address of the board upon rebooting
	TCPPutString(sktHTTP, curHTTP.data);	
}

void HTTPPrint_status_ok(void)
{
	if(lastSuccess)
		TCPPutROMString(sktHTTP, (ROM BYTE*)"block");
	else
		TCPPutROMString(sktHTTP, (ROM BYTE*)"none");
	lastSuccess = FALSE;
}

void HTTPPrint_status_fail(void)
{
	if(lastFailure)
		TCPPutROMString(sktHTTP, (ROM BYTE*)"block");
	else
		TCPPutROMString(sktHTTP, (ROM BYTE*)"none");
	lastFailure = FALSE;
}

void HTTPPrint_MEMDUMP(WORD memType)
{
	DWORD len;
	unsigned short long size;
	len = TCPIsPutReady(sktHTTP);

	if (memType == 0u) size = 256ul; //EEPROM 256 Bytes
	else if (memType == 1u) size = 4194304ul; //Flash xxx bytes
	else if (memType == 2u) size = 32768ul; //SRAM xxx bytes
	else return;

	if(curHTTP.callbackPos == 0u) curHTTP.callbackPos = size;

	while(len > 12 && curHTTP.callbackPos)
	{
		char data[1];
		if ((curHTTP.callbackPos & 63) == 0) {
			char lineNumber[7];
			sprintf(lineNumber, "%06lx", size - curHTTP.callbackPos);
			//Start of line
			TCPPutROMString(sktHTTP, (ROM BYTE*)"<br>");
			TCPPutString(sktHTTP, lineNumber);
			len -= 10;
		}

		//Continue line
		TCPPutROMString(sktHTTP, (ROM BYTE*)" ");
		len--;

		if (memType == 0) data[0] = eepromReadByte(size - curHTTP.callbackPos);
		else if (memType == 1) SPIFlashReadArray(size - curHTTP.callbackPos, data, 1);
		else if (memType == 2) data[0] = sramReadByte(size - curHTTP.callbackPos);
		
		len -= TCPPut(sktHTTP, btohexa_high(data[0]));
		len -= TCPPut(sktHTTP, btohexa_low(data[0]));

		curHTTP.callbackPos--;
	}
	return;
}

void HTTPPrint_FLASHSTATUS(void)
{
	unsigned char string[6];
	itoa(SPIFlashRDSR(), string);
	TCPPutString(sktHTTP, string);
}

void HTTPPrint_config_httpPort(void){
	BYTE digits[6];
	uitoa(WebSrvConfig.HTTPPort, digits);
	TCPPutString(sktHTTP, digits);
}

void HTTPPrint_config_httpsPort(void){
	BYTE digits[6];
	uitoa(WebSrvConfig.HTTPSPort, digits);
	TCPPutString(sktHTTP, digits);
}

void HTTPPrint_config_reqhttps(void){
	if(WebSrvConfig.Flags.DataRequireHTTPS)
		TCPPutROMString(sktHTTP, (ROM BYTE*)"checked");
	return;
}

void HTTPPrint_config_user(void){
	TCPPutString(sktHTTP, WebSrvConfig.AuthUser);
	return;
}

void HTTPPrint_config_pass(void){
	TCPPutString(sktHTTP, WebSrvConfig.AuthPwd);
	return;
}

void HTTPPrint_config_reqauth(void){
	if(WebSrvConfig.Flags.DataRequireAuth)
		TCPPutROMString(sktHTTP, (ROM BYTE*)"checked");
	return;
}

