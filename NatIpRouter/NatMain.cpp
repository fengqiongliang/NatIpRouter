#include "stdafx.h"
#include <stdio.h>
#include <winsock2.h>
#include <iphlpapi.h>
#include "NatMain.h"
#include "NatAdapter.h"
#include "NatTable.h"
#include "NatNdisApi.h"
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#define infoLenth 300

void restartNat();
void printInAddr(in_addr ip, char* outStr);
DWORD WINAPI threadRun(LPVOID lpThreadParameter);

HANDLE stopEvent = NULL;
HANDLE breakEvent = NULL;

void natMain(char* cliMac) {
	if (cliMac != NULL) {
		int len = ::strlen(cliMac);
		//::_strupr_s(cliMac, len);
		::strcpy_s(clientMac, macLenth, cliMac);
	}
	/*
	restartNat();
	::Sleep(5000);
	restartNat();
	::Sleep(5000);
	restartNat();
	while (true) {}
	*/
	restartNat();
	OVERLAPPED overlap = { 0 };
	HANDLE hand = NULL;
	overlap.hEvent = ::WSACreateEvent();
	HANDLE h_currThread = ::GetCurrentThread();
	DWORD dThreadId = ::GetThreadId(h_currThread);

	stopEvent = ::WSACreateEvent();
	HANDLE events[2];
	events[0] = stopEvent;
	events[1] = overlap.hEvent;
	while (true) {
		NotifyAddrChange(&hand, &overlap);
		DWORD ret = WaitForMultipleObjects(2,events,false,INFINITE);
		if (ret == 0) break;
		restartNat();
		WSAResetEvent(overlap.hEvent);
	}

}

void natStop(){
	if (stopEvent != NULL) {
		::SetEvent(stopEvent);
		stopEvent = NULL;
	}
	if (breakEvent != NULL) {
		::SetEvent(breakEvent);
		breakEvent = NULL;
	}
}

void restartNat() {
	if (breakEvent != NULL) {
		::SetEvent(breakEvent);
		breakEvent = NULL;
	}
	getLocalAdapterIPs();
	char info[infoLenth] = { 0 };
	printInAddr(providerIp, info);
	printf("providerIp = %s \n", info);
	printInAddr(clientIp, info);
	printf("clientIp = %s \n", info);
	printInAddr(teamviewerIp, info);
	printf("teamviewerip = %s \n", info);
	printInAddr(openvpnIp, info);
	printf("openvpn = %s \n", info);
	if (providerIp.S_un.S_addr == 0) {
		printf("cannot find the provider ip \n");
		return;
	}
	if (clientIp.S_un.S_addr == 0 && teamviewerIp.S_un.S_addr == 0 && openvpnIp.S_un.S_addr == 0) {
		printf("cannot find the clientIp,teamviewerIp,openvpnIp\n");
		return;
	}
	::CreateThread(NULL, 0, threadRun, NULL, 0, NULL);
	
}
DWORD WINAPI threadRun(LPVOID lpThreadParameter) {
	printf("Thread[%d] start nat ... \n ",::GetCurrentThreadId());
	doSnat(breakEvent);
	printf("Thread[%d] start exit(0) \n ", ::GetCurrentThreadId());
	return 0;
}

void printInAddr(in_addr ip, char* outStr) {
	ZeroMemory(outStr, infoLenth);
	sprintf_s(outStr, infoLenth, "%d.%d.%d.%d", ip.S_un.S_un_b.s_b1, ip.S_un.S_un_b.s_b2, ip.S_un.S_un_b.s_b3, ip.S_un.S_un_b.s_b4);
}