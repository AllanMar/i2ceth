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

#if defined(STACK_USE_HTTP2_SERVER)

#include "TCPIP Stack/TCPIP.h"
#include "Main.h"		// Needed for SaveAppConfig() prototype

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
	if(strcmppgm2ram((char *)cUser,(ROM char *)"btnic") == 0
		&& strcmppgm2ram((char *)cPass, (ROM char *)"btnic") == 0)
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
	BYTE filename[20];
	int retValue;

	// Load the file name
	// Make sure BYTE filename[] above is large enough for your longest name
	MPFSGetFilename(curHTTP.file, filename, 20);
	

	if(!memcmppgm2ram(filename, "btnic.cgi", 9))
	{
		switch(curHTTP.smPost)
		{
			case SM_BTNIC_WAIT_TO_SEND:
				if (BTCommGetStatus() == BT_COMMSTATE_IDLE || BTCommGetStatus() == BT_COMMSTATE_TX)
				{
					retValue = BTCommTX(curHTTP.data);
					//If idle here then TX timeout occurred
					if (BTCommGetStatus() == BT_COMMSTATE_IDLE) 
					{
						strcpy(curHTTP.data, "TX TIMEOUT");
						return HTTP_IO_DONE;
					}
					if (retValue == 0)
						curHTTP.smPost = SM_BTNIC_WAIT_FOR_RESP;
				}
				return HTTP_IO_WAITING;
			case SM_BTNIC_WAIT_FOR_RESP:
				if (BTCommGetStatus() == BT_COMMSTATE_MSG) return HTTP_IO_DONE;
				//If idle here then WAIT timeout occurred
				if (BTCommGetStatus() == BT_COMMSTATE_IDLE)
				{
					strcpy(curHTTP.data, "WAIT TIMEOUT");
					return HTTP_IO_DONE;
				}
				
				return HTTP_IO_WAITING;
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
	
	if(!memcmppgm2ram(filename, "postfile.ext", 12))
	{

	}
	return HTTP_IO_DONE;
}

#endif //(use_post)

#endif

void HTTPPrint_BTVer(void)
{
	TCPPutROMString(sktHTTP, (ROM void*)"1.0");
}

void HTTPPrint_BTStatus(void)
{
	switch (BTCommGetStatus())
	{
		case BT_COMMSTATE_IDLE:
			TCPPutROMString(sktHTTP, (ROM void*)"IDLE");
			break;
		case BT_COMMSTATE_WAIT:
			TCPPutROMString(sktHTTP, (ROM void*)"WAIT");
			break;
		case BT_COMMSTATE_RX:
			TCPPutROMString(sktHTTP, (ROM void*)"RX");
			break;
		case BT_COMMSTATE_MSG:
			TCPPutROMString(sktHTTP, (ROM void*)"MSG");
			break;
		case BT_COMMSTATE_ASYNCRX:
			TCPPutROMString(sktHTTP, (ROM void*)"ASYNCRX");
			break;
		case BT_COMMSTATE_ASYNCMSG:
			TCPPutROMString(sktHTTP, (ROM void*)"ASYNCMSG");
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

	if(curHTTP.callbackPos == 0u) curHTTP.callbackPos = BTCommGetRspLen();

	while(len && curHTTP.callbackPos)
	{
		len -= TCPPut(sktHTTP, BTCommGetRsp());
		curHTTP.callbackPos--;
	}
	return;
}