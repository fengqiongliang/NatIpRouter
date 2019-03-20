#include "stdafx.h"
#include <stdio.h>
#include <unordered_set>
#include <unordered_map>
#include <winsock2.h>
#include <iphlpapi.h>
#include "NatTable.h"
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

#define NAT_TABLE_BEGIN 10000        // 10000
#define NAT_TABLE_END 65536          // 256*256
#define NAT_TIMEOUT 600              // This is timeout in seconds for which we keep the inactive entry in the NAT table until removal (10 minutes)
#define NAT_TABLE_CLEAR_SIZE 10000   // when table.size() > NAT_TABLE_CLEAR_SIZE ,then clear the expire item for keeping table small

static USHORT tcpPortCount = NAT_TABLE_BEGIN;  //for tcp table port count
static USHORT udpPortCount = NAT_TABLE_BEGIN;  //for udp table port count
static USHORT dnsPortCount = NAT_TABLE_BEGIN;  //for dns table port count
static USHORT icmpIdCount  = NAT_TABLE_BEGIN;   //for icmp table port count

static std::unordered_map<uint64_t, natEntry> tcpMap;
static std::unordered_map<uint64_t, natEntry> udpMap;
static std::unordered_map<uint64_t, natEntry> dnsMap;
static std::unordered_map<uint64_t, natEntry> icmpMap;


void clearMaps() {
	tcpMap.clear();
	udpMap.clear();
	dnsMap.clear();
	icmpMap.clear();
}


natEntry* getByPut(ULONG dest_ip, USHORT dest_port, ULONG src_ip, USHORT src_port, ULONG natIp, std::unordered_map<uint64_t, natEntry>* itemMap, USHORT* portOrIdCount,ULONG srcClientIp) {
	uint64_t key = ((uint64_t)dest_ip << 32) | ((uint32_t)dest_port << 16) | src_port;
	natEntry *val = NULL;
	//get now time
	SYSTEMTIME systime;
	FILETIME filetime;
	ULARGE_INTEGER	now;
	::GetSystemTime(&systime);
	::SystemTimeToFileTime(&systime, &filetime);
	now.HighPart = filetime.dwHighDateTime;
	now.LowPart = filetime.dwLowDateTime;

	try {
		val = &itemMap->at(key);
	}
	catch (const std::out_of_range& oor) {}
	if (val != NULL) {
		val->insertTime = now.QuadPart;
		uint64_t key2 = ((uint64_t)dest_ip << 32) | ((uint32_t)dest_port << 16) | val->natPortOrId;
		itemMap->at(key2).insertTime = now.QuadPart;
		return val;
	}
	//new item
	natEntry newItem;
	newItem.destIp = dest_ip;
	newItem.destPort = dest_port;
	newItem.originalSrcIp = src_ip;
	newItem.originalSrcPort = src_port;
	newItem.natIp = natIp;
	newItem.natPortOrId = *portOrIdCount;
	newItem.srcClientIp = srcClientIp;
	newItem.insertTime = now.QuadPart;
	*portOrIdCount = *portOrIdCount + 1;
	if (*portOrIdCount >= NAT_TABLE_END)*portOrIdCount = NAT_TABLE_BEGIN; //recyle to begin

	itemMap->insert(std::pair<uint64_t, natEntry>(key, newItem));

	//just for get convience 
	uint64_t key2 = ((uint64_t)dest_ip << 32) | ((uint32_t)dest_port << 16) | newItem.natPortOrId;
	itemMap->insert(std::pair<uint64_t, natEntry>(key2, newItem));

	natEntry* ret = &itemMap->at(key); //get back the item address in tcpmap for return
	if (itemMap->size() < NAT_TABLE_CLEAR_SIZE)return ret;

	//clear tcpMap for resizing
	for (std::unordered_map<uint64_t, natEntry>::iterator it = itemMap->begin(); it != itemMap->end();) {
		if (((now.QuadPart - it->second.insertTime) / 10000000) > NAT_TIMEOUT) {
			it = itemMap->erase(it);
		}
		else {
			it++;
		}
	}

	return ret;
}


natEntry* getByPutTcp(ULONG dest_ip, USHORT dest_port, ULONG src_ip, USHORT src_port, ULONG natIp,ULONG srcClientIp) {
	return getByPut(dest_ip, dest_port, src_ip, src_port, natIp, &tcpMap, &tcpPortCount,srcClientIp);
}

natEntry* getTcp(ULONG dest_ip, USHORT dest_port, USHORT nat_port) {
	uint64_t key = ((uint64_t)dest_ip << 32) | ((uint32_t)dest_port << 16) | nat_port;
	natEntry *val = NULL;
	try {
		val = &tcpMap.at(key);
	}
	catch (const std::out_of_range& oor) {}
	return val;
}

natEntry* getByPutUdp(ULONG dest_ip, USHORT dest_port, ULONG src_ip, USHORT src_port, ULONG natIp,ULONG srcClientIp) {
	return getByPut(dest_ip, dest_port, src_ip, src_port, natIp, &udpMap, &udpPortCount,srcClientIp);
}

natEntry* getUdp(ULONG dest_ip, USHORT dest_port, USHORT nat_port) {
	uint64_t key = ((uint64_t)dest_ip << 32) | ((uint32_t)dest_port << 16) | nat_port;
	natEntry *val = NULL;
	try {
		val = &udpMap.at(key);
	}
	catch (const std::out_of_range& oor) {}
	return val;
}


natEntry* getByPutDns(ULONG dest_ip, USHORT dest_port, ULONG src_ip, USHORT src_port, ULONG natIp,ULONG srcClientIp) {
	return getByPut(dest_ip, dest_port, src_ip, src_port, natIp, &dnsMap, &dnsPortCount,srcClientIp);
}

natEntry* getDns(ULONG dest_ip, USHORT dest_port, USHORT nat_port) {
	uint64_t key = ((uint64_t)dest_ip << 32) | ((uint32_t)dest_port << 16) | nat_port;
	natEntry *val = NULL;
	try {
		val = &dnsMap.at(key);
	}
	catch (const std::out_of_range& oor) {}
	return val;
}


natEntry* getByPutIcmp(ULONG dest_ip, ULONG src_ip, USHORT icmpId, ULONG natIp,ULONG srcClientIp) {
	uint64_t key = (((uint64_t)dest_ip << 32) | src_ip) + icmpId;
	natEntry *val = NULL;
	//get now time
	SYSTEMTIME systime;
	FILETIME filetime;
	ULARGE_INTEGER	now;
	::GetSystemTime(&systime);
	::SystemTimeToFileTime(&systime, &filetime);
	now.HighPart = filetime.dwHighDateTime;
	now.LowPart = filetime.dwLowDateTime;

	try {
		val = &icmpMap.at(key);
	}
	catch (const std::out_of_range& oor) {}
	if (val != NULL) {
		val->insertTime = now.QuadPart;
		uint64_t key2 = (((uint64_t)dest_ip << 32) | val->natIp) + val->natPortOrId;
		icmpMap.at(key2).insertTime = now.QuadPart;
		return val;
	}
	//new item
	natEntry newItem;
	newItem.destIp = dest_ip;
	newItem.destPort = icmpId;
	newItem.originalSrcIp = src_ip;
	newItem.originalSrcPort = icmpId;
	newItem.natIp = natIp;
	newItem.natPortOrId = icmpIdCount;
	newItem.srcClientIp = srcClientIp;
	newItem.insertTime = now.QuadPart;
	icmpIdCount = icmpIdCount + 1;
	if (icmpIdCount >= NAT_TABLE_END)icmpIdCount = NAT_TABLE_BEGIN; //recyle to begin

	icmpMap.insert(std::pair<uint64_t, natEntry>(key, newItem));

	//just for get convience 
	uint64_t key2 = (((uint64_t)dest_ip << 32) | natIp) + newItem.natPortOrId;
	icmpMap.insert(std::pair<uint64_t, natEntry>(key2, newItem));

	natEntry* ret = &icmpMap.at(key); //get back the item address in tcpmap for return
	if (icmpMap.size() < NAT_TABLE_CLEAR_SIZE)return ret;

	//clear tcpMap for resizing
	for (std::unordered_map<uint64_t, natEntry>::iterator it = icmpMap.begin(); it != icmpMap.end();) {
		if (((now.QuadPart - it->second.insertTime) / 10000000) > NAT_TIMEOUT) {
			it = icmpMap.erase(it);
		}
		else {
			it++;
		}
	}

	return ret;
}

natEntry* getIcmp(ULONG dest_ip, ULONG src_ip, USHORT nat_icmpId) {
	uint64_t key = (((uint64_t)dest_ip << 32) | src_ip) + nat_icmpId;
	natEntry *val = NULL;
	try {
		val = &icmpMap.at(key);
	}
	catch (const std::out_of_range& oor) {}
	return val;
}

