#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <unordered_set>
#include <winsock2.h>
#include <iphlpapi.h>
#include "NatAdapter.h"
#include <iostream>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")



char teamviewerName[macLenth] = "teamviewer";
char openvpnName[macLenth] = "tap";

char providerMac[macLenth] = { 0 };
char clientMac[macLenth] = { 0 };
char teamviewerMac[macLenth] = { 0 };
char openvpnMac[macLenth] = { 0 };

in_addr providerIp = { 0 };   //provider ip
in_addr clientIp = { 0 };     //client ip
in_addr teamviewerIp = { 0 }; //teamviewer ip
in_addr openvpnIp = { 0 };    //openvpn ip
in_addr dnsIp = { 0 };        //dns ip

static std::unordered_set<ULONG> localIps;

in_addr parserIp(char* ipString);
int getWanInterfaceIndexByRouter();


int getWanInterfaceIndexByRouter() {
	int result = -1;

	PMIB_IPFORWARDTABLE pIpForwardTable = (MIB_IPFORWARDTABLE *)malloc(sizeof(MIB_IPFORWARDTABLE));
	DWORD dwSize = 0;
	DWORD dwRetVal = 0;
	
	//first call to get size
	if (::GetIpForwardTable(pIpForwardTable, &dwSize, true) == ERROR_INSUFFICIENT_BUFFER) {
		free(pIpForwardTable);
		pIpForwardTable = (MIB_IPFORWARDTABLE *)malloc(dwSize);
	}

	//second call to get data
	::GetIpForwardTable(pIpForwardTable, &dwSize, true);

	int defaultRouter = 0;
	for (int i = 0; i < (int)pIpForwardTable->dwNumEntries; i++) {
		ULONG dest = pIpForwardTable->table[i].dwForwardDest;
		ULONG mask = pIpForwardTable->table[i].dwForwardMask;
		int index  = pIpForwardTable->table[i].dwForwardIfIndex;
		bool isDefaultRouter = (dest == 0 && mask == 0) ? true : false;
		if (isDefaultRouter) {
			result = index;
			defaultRouter++;
		}
	}
	if (defaultRouter > 1) {
		printf("find more than 1 defalt router , cannot find the right wan interface \n");
		result = -1;
	}

	free(pIpForwardTable);

	return result;

}


void getLocalAdapterIPs() {
	int wanInterfaceIndex = getWanInterfaceIndexByRouter();
	HANDLE h_currThread = ::GetCurrentThread();
	DWORD dThreadId = ::GetThreadId(h_currThread);
	printf("Thread[%d] going to retrieve ip information. \n ", dThreadId);

	//adapter ip address
	DWORD				dwOutputBufferZize = 0;
	PIP_ADAPTER_INFO	pAdapterInfo = NULL;
	GetAdaptersInfo(pAdapterInfo, &dwOutputBufferZize);
	// Allocate required amount of memory from the heap
	pAdapterInfo = (PIP_ADAPTER_INFO)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwOutputBufferZize);
	if (GetAdaptersInfo(pAdapterInfo, &dwOutputBufferZize) != ERROR_SUCCESS) {
		printf("error : not enough memory for retrive ip information !! \n");
		exit(-2);
	}

	//initializing back to initializing value
	ZeroMemory(providerMac,macLenth);
	ZeroMemory(teamviewerMac,macLenth);
	ZeroMemory(openvpnMac,macLenth);
	ZeroMemory(&providerIp,sizeof(in_addr));
	ZeroMemory(&clientIp,sizeof(in_addr));
	ZeroMemory(&teamviewerIp,sizeof(in_addr));
	ZeroMemory(&openvpnIp,sizeof(in_addr));
	localIps.clear();

	for (PIP_ADAPTER_INFO pAdatper = pAdapterInfo; pAdatper != NULL; pAdatper = pAdatper->Next) {

		int index = pAdatper->Index;

		char name[200] = { 0 };
		char *strIp = pAdatper->IpAddressList.IpAddress.String;
		char *strMask = pAdatper->IpAddressList.IpMask.String;
		char *strGw = pAdatper->GatewayList.IpAddress.String;
		char pAdatperMac[20] = { 0 };
		sprintf_s(pAdatperMac, "%02X-%02X-%02X-%02X-%02X-%02X", pAdatper->Address[0], pAdatper->Address[1], pAdatper->Address[2], pAdatper->Address[3], pAdatper->Address[4], pAdatper->Address[5]);
		::strcpy_s(name, pAdatper->Description);
		::_strlwr_s(name);
		std::string sname(name);

		//Dns		
		IP_PER_ADAPTER_INFO* pPerAdapt = NULL;
		ULONG ulLen = 0;
		//first get Length
		::GetPerAdapterInfo(index, pPerAdapt, &ulLen);
		pPerAdapt = (IP_PER_ADAPTER_INFO*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ulLen);
		//second call get data
		::GetPerAdapterInfo(index, pPerAdapt, &ulLen);
		char *strdns = pPerAdapt->DnsServerList.IpAddress.String;
		

		printf("pAdatper.type = %d \n ", pAdatper->Type);
		printf("pAdatper.AdapterName = %s \n ", pAdatper->AdapterName);
		printf("pAdatper.Description = %s \n ", pAdatper->Description);
		printf("pAdatper.Address = %02X-%02X-%02X-%02X-%02X-%02X \n ", pAdatperMac);
		printf("name = %s \n ", name);
		printf("ip = %s \n ", strIp);
		printf("mask = %s \n ", strMask);
		printf("gw = %s \n ", strGw);
		printf("dns = %s \n ", strdns);

		in_addr ip = parserIp(strIp);
		if (index == wanInterfaceIndex) {
			::strcpy_s(providerMac, pAdatperMac);
			providerIp = parserIp(strIp);
		}
		if (::_strcmpi(pAdatperMac, clientMac) == 0){
			//::strcpy_s(clientMac, pAdatperMac);
			clientIp = parserIp(strIp);
		}
		
		if (sname.find(teamviewerName) == 0){
			::strcpy_s(teamviewerMac, pAdatperMac);
			teamviewerIp = parserIp(strIp);
		}
		if (sname.find(openvpnName) == 0){
			::strcpy_s(openvpnMac, pAdatperMac);
			openvpnIp = parserIp(strIp);
		}
		if (!::_strcmpi(strdns, "") == 0){
			dnsIp = parserIp(strdns);
		}
		if (ip.S_un.S_addr > 0 && ip.S_un.S_addr < 0xFFFFFFFF)localIps.insert(ip.S_un.S_addr);

		HeapFree(GetProcessHeap(), HEAP_ZERO_MEMORY, pPerAdapt);
	}
	
	// Free buffer
	HeapFree(GetProcessHeap(), 0, pAdapterInfo);
}


in_addr parserIp(char* ipString) {
	in_addr ip = { 0 };
	char ip_strPart1[4] = { 0 };
	char ip_strPart2[4] = { 0 };
	char ip_strPart3[4] = { 0 };
	char ip_strPart4[4] = { 0 };

	int dotIndex = 0;
	int part1Index = 0;
	int part2Index = 0;
	int part3Index = 0;
	int part4Index = 0;
	for (int i = 0; i < 16; i++) {
		if (ipString[i] == '.') {
			dotIndex++;
			continue;
		}
		if (0 <= dotIndex && dotIndex < 1 && part1Index < 3)ip_strPart1[part1Index++] = ipString[i];
		if (1 <= dotIndex && dotIndex < 2 && part2Index < 3)ip_strPart2[part2Index++] = ipString[i];
		if (2 <= dotIndex && dotIndex < 3 && part3Index < 3)ip_strPart3[part3Index++] = ipString[i];
		if (3 <= dotIndex && dotIndex < 4 && part4Index < 3)ip_strPart4[part4Index++] = ipString[i];
	}
	
	ip.S_un.S_un_b.s_b1 = (unsigned char)std::atoi(ip_strPart1);
	ip.S_un.S_un_b.s_b2 = (unsigned char)std::atoi(ip_strPart2);
	ip.S_un.S_un_b.s_b3 = (unsigned char)std::atoi(ip_strPart3);
	ip.S_un.S_un_b.s_b4 = (unsigned char)std::atoi(ip_strPart4);
	return ip;
}

bool isLocalIp(ULONG ip) {
	return localIps.count(ip) > 0;
}