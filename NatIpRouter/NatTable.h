#pragma once
#include <stdint.h>
#include <Windows.h>

typedef struct {
	ULONG destIp;
	USHORT destPort;
	ULONG originalSrcIp;
	USHORT originalSrcPort;
	ULONG natIp;
	USHORT natPortOrId;  //tcp/udp just for port ,but Icmp for id
	ULONG srcClientIp;   //from client ip to determinate which is client/teamviewer/openvpn
	ULONGLONG insertTime;
} natEntry, *pnatEntry;

void clearMaps();
natEntry* getTcp(ULONG dest_ip, USHORT dest_port, USHORT nat_port);
natEntry* getUdp(ULONG dest_ip, USHORT dest_port, USHORT nat_port);
natEntry* getDns(ULONG dest_ip, USHORT dest_port, USHORT nat_port);
natEntry* getIcmp(ULONG dest_ip, ULONG src_ip, USHORT nat_icmpId);
natEntry* getByPutTcp(ULONG dest_ip, USHORT dest_port, ULONG src_ip, USHORT src_port, ULONG natIp,ULONG srcClientIp);
natEntry* getByPutUdp(ULONG dest_ip, USHORT dest_port, ULONG src_ip, USHORT src_port, ULONG natIp,ULONG srcClientIp);
natEntry* getByPutDns(ULONG dest_ip, USHORT dest_port, ULONG src_ip, USHORT src_port, ULONG natIp,ULONG srcClientIp);
natEntry* getByPutIcmp(ULONG dest_ip, ULONG src_ip, USHORT icmpId, ULONG natIp,ULONG srcClientIp);

