#pragma once
#include <Windows.h>

#define macLenth 100

extern char teamviewerName[macLenth];
extern char openvpnName[macLenth];

extern char providerMac[macLenth];
extern char clientMac[macLenth];
extern char teamviewerMac[macLenth];
extern char openvpnMac[macLenth];

extern in_addr providerIp;   //provider ip
extern in_addr clientIp;     //client ip
extern in_addr teamviewerIp; //teamviewer ip
extern in_addr openvpnIp;    //openvpn ip
extern in_addr dnsIp;        //dns ip

bool isLocalIp(ULONG ip);  
void getLocalAdapterIPs(); //retrive local adatper information and set global variable providerIp,clientIp,teamviewerIp,openvpnIp
