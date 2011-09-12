/**************************************************************
 * HTTPPrint.h
 * Provides callback headers and resolution for user's custom
 * HTTP Application.
 * 
 * This file is automatically generated by the MPFS Utility
 * ALL MODIFICATIONS WILL BE OVERWRITTEN BY THE MPFS GENERATOR
 **************************************************************/

#ifndef __HTTPPRINT_H
#define __HTTPPRINT_H

#include "TCPIP Stack/TCPIP.h"

#if defined(STACK_USE_HTTP2_SERVER)

extern HTTP_STUB httpStubs[MAX_HTTP_CONNECTIONS];
extern BYTE curHTTPID;

void HTTPPrint(DWORD callbackID);
void HTTPPrint_BTVer(void);
void HTTPPrint_BTNic_CGI(void);
void HTTPPrint_SSP1CON1(void);
void HTTPPrint_SSP1CON2(void);
void HTTPPrint_SSP1STAT(void);
void HTTPPrint_BTCommTimer(void);
void HTTPPrint_TickGet(void);
void HTTPPrint_BTState(void);
void HTTPPrint_BTBuffer(void);
void HTTPPrint_status_fail(void);
void HTTPPrint_config_mac(void);
void HTTPPrint_config_hostname(void);
void HTTPPrint_config_dhcpchecked(void);
void HTTPPrint_config_ip(void);
void HTTPPrint_config_gw(void);
void HTTPPrint_config_subnet(void);
void HTTPPrint_config_dns1(void);
void HTTPPrint_config_dns2(void);
void HTTPPrint_reboot(void);
void HTTPPrint_EEPCFG(void);

void HTTPPrint(DWORD callbackID)
{
	switch(callbackID)
	{
        case 0x00000001:
			HTTPPrint_BTVer();
			break;
        case 0x00000002:
			HTTPPrint_BTNic_CGI();
			break;
        case 0x00000004:
			HTTPPrint_SSP1CON1();
			break;
        case 0x00000005:
			HTTPPrint_SSP1CON2();
			break;
        case 0x00000006:
			HTTPPrint_SSP1STAT();
			break;
        case 0x00000007:
			HTTPPrint_BTCommTimer();
			break;
        case 0x00000008:
			HTTPPrint_TickGet();
			break;
        case 0x00000009:
			HTTPPrint_BTState();
			break;
        case 0x0000000a:
			HTTPPrint_BTBuffer();
			break;
        case 0x0000000c:
			HTTPPrint_status_fail();
			break;
        case 0x0000000d:
			HTTPPrint_config_mac();
			break;
        case 0x0000000e:
			HTTPPrint_config_hostname();
			break;
        case 0x0000000f:
			HTTPPrint_config_dhcpchecked();
			break;
        case 0x00000010:
			HTTPPrint_config_ip();
			break;
        case 0x00000011:
			HTTPPrint_config_gw();
			break;
        case 0x00000012:
			HTTPPrint_config_subnet();
			break;
        case 0x00000013:
			HTTPPrint_config_dns1();
			break;
        case 0x00000014:
			HTTPPrint_config_dns2();
			break;
        case 0x00000015:
			HTTPPrint_reboot();
			break;
        case 0x00000017:
			HTTPPrint_EEPCFG();
			break;
		default:
			// Output notification for undefined values
			TCPPutROMArray(sktHTTP, (ROM BYTE*)"!DEF", 4);
	}

	return;
}

void HTTPPrint_(void)
{
	TCPPut(sktHTTP, '~');
	return;
}

#endif

#endif
