#include "stdafx.h"
#include "iphlp.h"
#include <string>
#include "NatTable.h";
#include "NatAdapter.h"
#include "ndisapi.h"

void providerRun(CNdisApi &ndisApi, ETH_REQUEST &ethPacket);
void clientRun(CNdisApi &ndisApi, ETH_REQUEST &ethPacket,ULONG srcClientIp);
void handleTcpPacketByProvider(CNdisApi &ndisApi, ETH_REQUEST &ethPacket);
void handleUdpPacketByProvider(CNdisApi &ndisApi, ETH_REQUEST &ethPacket);
void handleIcmpPacketByProvider(CNdisApi &ndisApi, ETH_REQUEST &ethPacket);
void handleTcpPacketByClient(CNdisApi &ndisApi, ETH_REQUEST &ethPacket,ULONG srcClientIp);
void handleUdpPacketByClient(CNdisApi &ndisApi, ETH_REQUEST &ethPacket,ULONG srcClientIp);
void handleDnsPacketByClient(CNdisApi &ndisApi, ETH_REQUEST &ethPacket,ULONG srcClientIp);
void handleIcmpPacketByClient(CNdisApi &ndisApi, ETH_REQUEST &ethPacket,ULONG srcClientIp);




using namespace std;

void doSnat(HANDLE& breakEvent) {
	HANDLE hProvider = NULL;    //provider is to providing snat(本地连接)
	HANDLE hClient = NULL;      //client is to be snat()
	HANDLE hTeamviewer = NULL;  //client is to be snat(TeamviewVPN)
	HANDLE hOpenvpn = NULL;     //client is to be snat(OpenVPN)

	CNdisApi ndisApi;
	if (!ndisApi.IsDriverLoaded()) {
		printf("window packet filter not install ,please install \n");
		return;
	}

	ULONG version = ndisApi.GetVersion();
	printf("version = %d \n", version);


	TCP_AdapterList adapterList;
	ndisApi.GetTcpipBoundAdaptersInfo(&adapterList);
	for (int i = 0; i < adapterList.m_nAdapterCount; i++) {
		//mac address
		char mac[100] = { 0 };
		sprintf_s(mac, "%02X-%02X-%02X-%02X-%02X-%02X", adapterList.m_czCurrentAddress[i][0], adapterList.m_czCurrentAddress[i][1], adapterList.m_czCurrentAddress[i][2], adapterList.m_czCurrentAddress[i][3], adapterList.m_czCurrentAddress[i][4], adapterList.m_czCurrentAddress[i][5]);
		
		if (_stricmp(providerMac, mac) == 0)hProvider = adapterList.m_nAdapterHandle[i];
		if (_stricmp(clientMac, mac) == 0) hClient = adapterList.m_nAdapterHandle[i];
		if (_stricmp(teamviewerMac, mac) == 0) hTeamviewer = adapterList.m_nAdapterHandle[i];
		if (_stricmp(openvpnMac, mac) == 0) hOpenvpn = adapterList.m_nAdapterHandle[i];
	}

	if (hProvider == NULL || (hClient == NULL && hTeamviewer == NULL && hOpenvpn == NULL)) {
		printf("can not find provider adapter or (client/teamviewer/openvpn) adapter exit(-3)\n");
		exit(-3);
	}

	ADAPTER_MODE providerMode;
	//providerMode.dwFlags = MSTCP_FLAG_RECV_TUNNEL;
	//providerMode.dwFlags = MSTCP_FLAG_SENT_TUNNEL | MSTCP_FLAG_RECV_TUNNEL;
	providerMode.dwFlags = MSTCP_FLAG_SENT_TUNNEL|MSTCP_FLAG_RECV_TUNNEL|MSTCP_FLAG_LOOPBACK_BLOCK|MSTCP_FLAG_LOOPBACK_FILTER;
	//providerMode.dwFlags = MSTCP_FLAG_SENT_LISTEN | MSTCP_FLAG_RECV_LISTEN ;
	providerMode.hAdapterHandle = hProvider;
	
	ADAPTER_MODE clientMode;
	//clientMode.dwFlags = MSTCP_FLAG_RECV_TUNNEL;
	//clientMode.dwFlags = MSTCP_FLAG_SENT_TUNNEL | MSTCP_FLAG_RECV_TUNNEL;
	clientMode.dwFlags = MSTCP_FLAG_SENT_TUNNEL|MSTCP_FLAG_RECV_TUNNEL|MSTCP_FLAG_LOOPBACK_BLOCK|MSTCP_FLAG_LOOPBACK_FILTER;
	//clientMode.dwFlags = MSTCP_FLAG_SENT_LISTEN | MSTCP_FLAG_RECV_LISTEN;
	clientMode.hAdapterHandle = hClient;

	ADAPTER_MODE teamviewerMode;
	//teamviewerMode.dwFlags = MSTCP_FLAG_RECV_TUNNEL;
	//teamviewerMode.dwFlags = MSTCP_FLAG_SENT_TUNNEL | MSTCP_FLAG_RECV_TUNNEL;
	teamviewerMode.dwFlags = MSTCP_FLAG_SENT_TUNNEL|MSTCP_FLAG_RECV_TUNNEL|MSTCP_FLAG_LOOPBACK_BLOCK|MSTCP_FLAG_LOOPBACK_FILTER;
	//teamviewerMode.dwFlags = MSTCP_FLAG_SENT_LISTEN | MSTCP_FLAG_RECV_LISTEN;
	teamviewerMode.hAdapterHandle = hTeamviewer;

	ADAPTER_MODE openvpnMode;

	//openvpnMode.dwFlags = MSTCP_FLAG_RECV_TUNNEL;
	//openvpnMode.dwFlags = MSTCP_FLAG_SENT_TUNNEL | MSTCP_FLAG_RECV_TUNNEL;
	openvpnMode.dwFlags = MSTCP_FLAG_SENT_TUNNEL|MSTCP_FLAG_RECV_TUNNEL|MSTCP_FLAG_LOOPBACK_BLOCK|MSTCP_FLAG_LOOPBACK_FILTER;
	//openvpnMode.dwFlags = MSTCP_FLAG_SENT_LISTEN | MSTCP_FLAG_RECV_LISTEN;
	openvpnMode.hAdapterHandle = hOpenvpn;

	HANDLE events[5] = { 0 };
	breakEvent = ::CreateEvent(NULL, true, false, NULL);
	HANDLE hProviderEvent = ::CreateEvent(NULL, true, false, NULL);
	HANDLE hClientEvent = ::CreateEvent(NULL, true, false, NULL);
	HANDLE hTeamviewerEvent = ::CreateEvent(NULL, true, false, NULL);
	HANDLE hOpenvpnEvent = ::CreateEvent(NULL, true, false, NULL);
	events[0] = breakEvent;
	events[1] = hProviderEvent;
	events[2] = hClientEvent;
	events[3] = hTeamviewerEvent;
	events[4] = hOpenvpnEvent;

	
	if(hProvider != NULL){
		ndisApi.SetPacketEvent(hProvider, hProviderEvent);
		ndisApi.SetAdapterMode(&providerMode);
	}
	if(hClient != NULL){
		ndisApi.SetPacketEvent(hClient, hClientEvent);
		ndisApi.SetAdapterMode(&clientMode);
	}
	if(hTeamviewer != NULL){
		ndisApi.SetPacketEvent(hTeamviewer, hTeamviewerEvent);
		ndisApi.SetAdapterMode(&teamviewerMode);
	}
	if(hOpenvpn != NULL){
		ndisApi.SetPacketEvent(hOpenvpn, hOpenvpnEvent);
		ndisApi.SetAdapterMode(&openvpnMode);
	}


	ETH_REQUEST packet;
	//NDISRD_ETH_Packet ethpacket;
	INTERMEDIATE_BUFFER buf;
	//ethpacket.Buffer = &buf;
	//packet.EthPacket = ethpacket;

	ZeroMemory(&packet, sizeof(ETH_REQUEST));
	ZeroMemory(&buf, sizeof(INTERMEDIATE_BUFFER));
	packet.EthPacket.Buffer = &buf;
	clearMaps();
	do {
		DWORD dwIndex = -1;
		//printf("waiting packet ... \n", dwIndex);
		dwIndex = ::WaitForMultipleObjects(5, events, false, INFINITE);
		//printf("dwIndex = %d packet arriving\n",dwIndex);
		if (dwIndex == 0) break;
		if (dwIndex == 1) {
			packet.hAdapterHandle = hProvider;
			providerRun(ndisApi,packet);
			::ResetEvent(hProviderEvent);
		}
		if (dwIndex == 2) {//client
			packet.hAdapterHandle = hClient;
			clientRun(ndisApi, packet,clientIp.S_un.S_addr);
			::ResetEvent(hClientEvent);
		}
		if (dwIndex == 3) {//teamviewvpn
			packet.hAdapterHandle = hTeamviewer;
			clientRun(ndisApi, packet,teamviewerIp.S_un.S_addr);
			::ResetEvent(hTeamviewerEvent);
		}
		if (dwIndex == 4) {//openvpn
			packet.hAdapterHandle = hOpenvpn;
			clientRun(ndisApi, packet,openvpnIp.S_un.S_addr);
			::ResetEvent(hOpenvpnEvent);
		}
	} while (true);


	//cleaning

	printf("Thread[%d] exit(0)/n",::GetCurrentThreadId());

}



void providerRun(CNdisApi &ndisApi, ETH_REQUEST &ethPacket) {

	while (ndisApi.ReadPacket(&ethPacket)) {
		ether_header_ptr pEthernetHead = (ether_header*)ethPacket.EthPacket.Buffer->m_IBuffer;

		if (ntohs(pEthernetHead->h_proto) == ETH_P_IP) {
			iphdr_ptr pIpHead = (iphdr_ptr)((PUCHAR)ethPacket.EthPacket.Buffer->m_IBuffer + ETHER_HEADER_LENGTH);

			bool isSend = (ethPacket.EthPacket.Buffer->m_dwDeviceFlags == PACKET_FLAG_ON_SEND);
			bool isReceive = (ethPacket.EthPacket.Buffer->m_dwDeviceFlags == PACKET_FLAG_ON_RECEIVE);
			//go ahead any src = sProvider address which just send by myself ,can not be forwarding
			bool isSrcLocalIp = isLocalIp(pIpHead->ip_src.S_un.S_addr);
			bool dstIpIsProviderIp = (providerIp.S_un.S_addr == pIpHead->ip_dst.S_un.S_addr);

#ifdef _DEBUG
			SYSTEMTIME stime;
			::GetSystemTime(&stime);
			char ctime[100];
			sprintf_s(ctime,"%d:%d",stime.wMinute,stime.wSecond);
			CHAR sIpInfo[500] = { 0 };
			sprintf_s(sIpInfo, "%d.%d.%d.%d --> %d.%d.%d.%d",
				pIpHead->ip_src.S_un.S_un_b.s_b1,
				pIpHead->ip_src.S_un.S_un_b.s_b2,
				pIpHead->ip_src.S_un.S_un_b.s_b3,
				pIpHead->ip_src.S_un.S_un_b.s_b4,
				pIpHead->ip_dst.S_un.S_un_b.s_b1,
				pIpHead->ip_dst.S_un.S_un_b.s_b2,
				pIpHead->ip_dst.S_un.S_un_b.s_b3,
				pIpHead->ip_dst.S_un.S_un_b.s_b4);
			string msg;
			msg.append(sIpInfo);
			if (isSend)msg += " send ";
			if (isReceive)msg += " receive ";
			if (pIpHead->ip_p == IPPROTO_TCP)msg += " tcp ";
			if (pIpHead->ip_p == IPPROTO_UDP)msg += " udp ";
			if (pIpHead->ip_p == IPPROTO_ICMP)msg += " icmp ";
			printf("%s[provier]%s\n", ctime,msg.data());

#endif
		
			if (!isSrcLocalIp && isReceive && dstIpIsProviderIp && pIpHead->ip_p == IPPROTO_TCP)  handleTcpPacketByProvider(ndisApi,ethPacket);
			if (!isSrcLocalIp && isReceive && dstIpIsProviderIp && pIpHead->ip_p == IPPROTO_UDP)  handleUdpPacketByProvider(ndisApi, ethPacket);
			if (!isSrcLocalIp && isReceive && dstIpIsProviderIp && pIpHead->ip_p == IPPROTO_ICMP) handleIcmpPacketByProvider(ndisApi, ethPacket);


		}


		if (ethPacket.EthPacket.Buffer->m_dwDeviceFlags == PACKET_FLAG_ON_SEND) {
			ndisApi.SendPacketToAdapter(&ethPacket);
		}else {
			ndisApi.SendPacketToMstcp(&ethPacket);
		}


	}



}

void handleTcpPacketByProvider(CNdisApi &ndisApi, ETH_REQUEST &ethPacket) {
	iphdr_ptr pIpHead = (iphdr_ptr)((PUCHAR)ethPacket.EthPacket.Buffer->m_IBuffer + ETHER_HEADER_LENGTH);
	tcphdr_ptr pTcpHead = (tcphdr_ptr)((PUCHAR)pIpHead + (pIpHead->ip_hl * 32 / 8));
	PUCHAR pTcpData = (PUCHAR)((PUCHAR)pTcpHead + sizeof(tcphdr));
	int iTcpDataLen = ntohs(pIpHead->ip_len) - (pIpHead->ip_hl * 32 / 8) - sizeof(tcphdr);

#ifdef _DEBUG
	CHAR sIpInfo[500] = { 0 };
	sprintf_s(sIpInfo, "%d.%d.%d.%d:%d --> %d.%d.%d.%d:%d [id=%d]",
		pIpHead->ip_src.S_un.S_un_b.s_b1,
		pIpHead->ip_src.S_un.S_un_b.s_b2,
		pIpHead->ip_src.S_un.S_un_b.s_b3,
		pIpHead->ip_src.S_un.S_un_b.s_b4,
		ntohs(pTcpHead->th_sport),
		pIpHead->ip_dst.S_un.S_un_b.s_b1,
		pIpHead->ip_dst.S_un.S_un_b.s_b2,
		pIpHead->ip_dst.S_un.S_un_b.s_b3,
		pIpHead->ip_dst.S_un.S_un_b.s_b4,
		ntohs(pTcpHead->th_dport),
		ntohs(pIpHead->ip_id));
	printf("[provier]%s tcp receive\n ", sIpInfo);
#endif

	ULONG dest_ip = pIpHead->ip_src.S_un.S_addr;
	USHORT dest_port = ntohs(pTcpHead->th_sport);
	USHORT nat_port = ntohs(pTcpHead->th_dport);
	natEntry *natEntry = getTcp(dest_ip, dest_port, nat_port);

	if (natEntry != NULL) {
		pIpHead->ip_dst.S_un.S_addr = natEntry->originalSrcIp;   //recover unnat dest ip   address
		pTcpHead->th_dport = htons(natEntry->originalSrcPort);   //recover unant dest port address

		ndisApi.RecalculateTCPChecksum(ethPacket.EthPacket.Buffer);
		ndisApi.RecalculateIPChecksum(ethPacket.EthPacket.Buffer);
	}


}

void handleUdpPacketByProvider(CNdisApi &ndisApi, ETH_REQUEST &ethPacket) {
	iphdr_ptr pIpHead = (iphdr_ptr)((PUCHAR)ethPacket.EthPacket.Buffer->m_IBuffer + ETHER_HEADER_LENGTH);
	udphdr_ptr pUdpHead = (udphdr_ptr)((PUCHAR)pIpHead + (pIpHead->ip_hl * 32 / 8));

#ifdef _DEBUG
	CHAR sIpInfo[500] = { 0 };
	sprintf_s(sIpInfo, "%d.%d.%d.%d:%d --> %d.%d.%d.%d:%d [id=%d]",
		pIpHead->ip_src.S_un.S_un_b.s_b1,
		pIpHead->ip_src.S_un.S_un_b.s_b2,
		pIpHead->ip_src.S_un.S_un_b.s_b3,
		pIpHead->ip_src.S_un.S_un_b.s_b4,
		ntohs(pUdpHead->th_sport),
		pIpHead->ip_dst.S_un.S_un_b.s_b1,
		pIpHead->ip_dst.S_un.S_un_b.s_b2,
		pIpHead->ip_dst.S_un.S_un_b.s_b3,
		pIpHead->ip_dst.S_un.S_un_b.s_b4,
		ntohs(pUdpHead->th_dport),
		ntohs(pIpHead->ip_id));
	printf("[provier]%s udp receive\n ", sIpInfo);
#endif

	ULONG dest_ip = pIpHead->ip_src.S_un.S_addr;
	USHORT dest_port = ntohs(pUdpHead->th_sport);
	USHORT nat_port = ntohs(pUdpHead->th_dport);
	natEntry *natEntry = getUdp(dest_ip, dest_port, nat_port);

	if (natEntry != NULL) {
		pIpHead->ip_dst.S_un.S_addr = natEntry->originalSrcIp;   //recover unnat dest ip   address
		pUdpHead->th_dport = htons(natEntry->originalSrcPort);   //recover unant dest port address
		//dns
		if(pIpHead->ip_src.S_un.S_addr == dnsIp.S_un.S_addr && ntohs(pUdpHead->th_sport) == 53){
			//local ip dns recover
			pIpHead->ip_src.S_un.S_addr = natEntry->srcClientIp;
		}

		ndisApi.RecalculateUDPChecksum(ethPacket.EthPacket.Buffer);
		ndisApi.RecalculateIPChecksum(ethPacket.EthPacket.Buffer);
	}

}


void handleIcmpPacketByProvider(CNdisApi &ndisApi, ETH_REQUEST &ethPacket) {
	iphdr_ptr pIpHead = (iphdr_ptr)((PUCHAR)ethPacket.EthPacket.Buffer->m_IBuffer + ETHER_HEADER_LENGTH);
	icmphdr_ptr pIcmpHead = (icmphdr_ptr)((PUCHAR)pIpHead + (pIpHead->ip_hl * 32 / 8));

#ifdef _DEBUG
	CHAR sIpInfo[500] = { 0 };
	sprintf_s(sIpInfo, "%d.%d.%d.%d:%d --> %d.%d.%d.%d:%d [id=%d]",
		pIpHead->ip_src.S_un.S_un_b.s_b1,
		pIpHead->ip_src.S_un.S_un_b.s_b2,
		pIpHead->ip_src.S_un.S_un_b.s_b3,
		pIpHead->ip_src.S_un.S_un_b.s_b4,
		ntohs(pIcmpHead->id),
		pIpHead->ip_dst.S_un.S_un_b.s_b1,
		pIpHead->ip_dst.S_un.S_un_b.s_b2,
		pIpHead->ip_dst.S_un.S_un_b.s_b3,
		pIpHead->ip_dst.S_un.S_un_b.s_b4,
		ntohs(pIcmpHead->id),
		ntohs(pIpHead->ip_id));
	printf("[provier]%s icmp receive\n ", sIpInfo);
#endif

	ULONG dest_ip = pIpHead->ip_src.S_un.S_addr;
	ULONG src_ip = pIpHead->ip_dst.S_un.S_addr;
	USHORT nat_icmpId = ntohs(pIcmpHead->id);
	natEntry *natEntry = getIcmp(dest_ip, src_ip, nat_icmpId);

	if (natEntry != NULL) {
		pIpHead->ip_dst.S_un.S_addr = natEntry->originalSrcIp;   //recover unnat dest ip   address
		pIcmpHead->id = htons(natEntry->originalSrcPort);		 //recover unant icmp id
		ndisApi.RecalculateICMPChecksum(ethPacket.EthPacket.Buffer);
		ndisApi.RecalculateIPChecksum(ethPacket.EthPacket.Buffer);
	}


}





void clientRun(CNdisApi &ndisApi,ETH_REQUEST &ethPacket,ULONG srcClientIp) {
		
		while (ndisApi.ReadPacket(&ethPacket)) {
			ether_header_ptr pEthernetHead = (ether_header*)ethPacket.EthPacket.Buffer->m_IBuffer;
			if (ntohs(pEthernetHead->h_proto) == ETH_P_IP) {
				iphdr_ptr pIpHead = (iphdr_ptr)((PUCHAR)ethPacket.EthPacket.Buffer->m_IBuffer + ETHER_HEADER_LENGTH);

				bool isSend = (ethPacket.EthPacket.Buffer->m_dwDeviceFlags == PACKET_FLAG_ON_SEND);
				bool isReceive = (ethPacket.EthPacket.Buffer->m_dwDeviceFlags == PACKET_FLAG_ON_RECEIVE);
				//go ahead any src = sClient address which just send by myself ,can not be forwarding
				bool isSrcLocalIp = isLocalIp(pIpHead->ip_src.S_un.S_addr);
				bool isDstLocalIp = isLocalIp(pIpHead->ip_dst.S_un.S_addr);

#ifdef _DEBUG
				SYSTEMTIME stime;
				::GetSystemTime(&stime);
				char ctime[100];
				sprintf_s(ctime,"%d:%d",stime.wMinute,stime.wSecond);
				CHAR sIpInfo[500] = { 0 };
				sprintf_s(sIpInfo, "%d.%d.%d.%d --> %d.%d.%d.%d",
					pIpHead->ip_src.S_un.S_un_b.s_b1,
					pIpHead->ip_src.S_un.S_un_b.s_b2,
					pIpHead->ip_src.S_un.S_un_b.s_b3,
					pIpHead->ip_src.S_un.S_un_b.s_b4,
					pIpHead->ip_dst.S_un.S_un_b.s_b1,
					pIpHead->ip_dst.S_un.S_un_b.s_b2,
					pIpHead->ip_dst.S_un.S_un_b.s_b3,
					pIpHead->ip_dst.S_un.S_un_b.s_b4); 
				string msg;
				msg.append(sIpInfo);
				if (isSend)msg += " send ";
				if (isReceive)msg += " receive ";
				if (pIpHead->ip_p == IPPROTO_TCP)msg += " tcp ";
				if (pIpHead->ip_p == IPPROTO_UDP)msg += " udp ";
				if (pIpHead->ip_p == IPPROTO_ICMP)msg += " icmp ";
				if(clientIp.S_un.S_addr == srcClientIp)msg += " client ";
				if(teamviewerIp.S_un.S_addr == srcClientIp)msg += " teamviewer ";
				if(openvpnIp.S_un.S_addr == srcClientIp)msg += " openvpn ";
				printf("%s[client]%s\n",ctime,msg.data());
#endif				
			
				if (!isSrcLocalIp && !isDstLocalIp && isReceive && pIpHead->ip_p == IPPROTO_TCP)  handleTcpPacketByClient(ndisApi,ethPacket,srcClientIp);
				if (!isSrcLocalIp && isReceive && pIpHead->ip_p == IPPROTO_UDP) handleUdpPacketByClient(ndisApi,ethPacket,srcClientIp);
				if (!isSrcLocalIp && isReceive && pIpHead->ip_p == IPPROTO_UDP) handleDnsPacketByClient(ndisApi,ethPacket,srcClientIp);
				if (!isSrcLocalIp && !isDstLocalIp && isReceive && pIpHead->ip_p == IPPROTO_ICMP) handleIcmpPacketByClient(ndisApi,ethPacket,srcClientIp);
				

			}
			
			
			if (ethPacket.EthPacket.Buffer->m_dwDeviceFlags == PACKET_FLAG_ON_SEND) {
				ndisApi.SendPacketToAdapter(&ethPacket);
			}else {
				ndisApi.SendPacketToMstcp(&ethPacket);
			}

			
			
		} 
	
}

void handleTcpPacketByClient(CNdisApi &ndisApi,ETH_REQUEST &ethPacket,ULONG srcClientIp) {
	iphdr_ptr pIpHead   = (iphdr_ptr)((PUCHAR)ethPacket.EthPacket.Buffer->m_IBuffer + ETHER_HEADER_LENGTH);
	tcphdr_ptr pTcpHead = (tcphdr_ptr)((PUCHAR)pIpHead + (pIpHead->ip_hl * 32 / 8));
	PUCHAR pTcpData     = (PUCHAR)((PUCHAR)pTcpHead + sizeof(tcphdr));
	int iTcpDataLen     = ntohs(pIpHead->ip_len) - (pIpHead->ip_hl * 32 / 8) - sizeof(tcphdr);
	
#ifdef _DEBUG
	CHAR sIpInfo[500] = { 0 };
	sprintf_s(sIpInfo, "%d.%d.%d.%d:%d --> %d.%d.%d.%d:%d [id=%d]",
		pIpHead->ip_src.S_un.S_un_b.s_b1,
		pIpHead->ip_src.S_un.S_un_b.s_b2,
		pIpHead->ip_src.S_un.S_un_b.s_b3,
		pIpHead->ip_src.S_un.S_un_b.s_b4,
		ntohs(pTcpHead->th_sport),
		pIpHead->ip_dst.S_un.S_un_b.s_b1,
		pIpHead->ip_dst.S_un.S_un_b.s_b2,
		pIpHead->ip_dst.S_un.S_un_b.s_b3,
		pIpHead->ip_dst.S_un.S_un_b.s_b4,
		ntohs(pTcpHead->th_dport),
		ntohs(pIpHead->ip_id));
	printf("[client]%s tcp reiceive\n ", sIpInfo);
#endif

	//save original information
	ULONG dest_ip    =  pIpHead->ip_dst.S_un.S_addr;
	USHORT dest_port =  ntohs(pTcpHead->th_dport);
	ULONG src_ip     =  pIpHead->ip_src.S_un.S_addr;
	USHORT src_port  =  ntohs(pTcpHead->th_sport);
	ULONG natIp      =  providerIp.S_un.S_addr;
	natEntry *natEntry = getByPutTcp(dest_ip, dest_port, src_ip, src_port, natIp,srcClientIp);

	pIpHead->ip_src.S_un.S_addr = natEntry->natIp;      //modify source ip address
	pTcpHead->th_sport = htons(natEntry->natPortOrId);  //modify source port address
		
	//printf("pTcpHead->th_sport = %d  tmp_nat_port = %d\n",ntohs(pTcpHead->th_sport),tmp_nat_port);
	ndisApi.RecalculateTCPChecksum(ethPacket.EthPacket.Buffer);
	ndisApi.RecalculateIPChecksum(ethPacket.EthPacket.Buffer);
		
	

}

void handleUdpPacketByClient(CNdisApi &ndisApi,ETH_REQUEST &ethPacket,ULONG srcClientIp) {
	iphdr_ptr pIpHead   = (iphdr_ptr)((PUCHAR)ethPacket.EthPacket.Buffer->m_IBuffer + ETHER_HEADER_LENGTH);
	udphdr_ptr pUdpHead = (udphdr_ptr)((PUCHAR)pIpHead + (pIpHead->ip_hl * 32 / 8));
	if(isLocalIp(pIpHead->ip_dst.S_un.S_addr) && ntohs(pUdpHead->th_dport) == 53)return;

#ifdef _DEBUG
	CHAR sIpInfo[500] = { 0 };
	sprintf_s(sIpInfo, "%d.%d.%d.%d:%d --> %d.%d.%d.%d:%d [id=%d]",
		pIpHead->ip_src.S_un.S_un_b.s_b1,
		pIpHead->ip_src.S_un.S_un_b.s_b2,
		pIpHead->ip_src.S_un.S_un_b.s_b3,
		pIpHead->ip_src.S_un.S_un_b.s_b4,
		ntohs(pUdpHead->th_sport),
		pIpHead->ip_dst.S_un.S_un_b.s_b1,
		pIpHead->ip_dst.S_un.S_un_b.s_b2,
		pIpHead->ip_dst.S_un.S_un_b.s_b3,
		pIpHead->ip_dst.S_un.S_un_b.s_b4,
		ntohs(pUdpHead->th_dport),
		ntohs(pIpHead->ip_id));
	printf("[client]%s udp reiceive\n ", sIpInfo);
#endif

	//save original information
	ULONG dest_ip    =  pIpHead->ip_dst.S_un.S_addr;
	USHORT dest_port =  ntohs(pUdpHead->th_dport);
	ULONG src_ip     =  pIpHead->ip_src.S_un.S_addr;
	USHORT src_port  =  ntohs(pUdpHead->th_sport);
	ULONG natIp      =  providerIp.S_un.S_addr;
	natEntry *natEntry = getByPutUdp(dest_ip, dest_port, src_ip, src_port, natIp,srcClientIp);
	
	pIpHead->ip_src.S_un.S_addr = natEntry->natIp;      //modify source ip address
	pUdpHead->th_sport = htons(natEntry->natPortOrId);  //modify source port address
		
	ndisApi.RecalculateUDPChecksum(ethPacket.EthPacket.Buffer);
	ndisApi.RecalculateIPChecksum(ethPacket.EthPacket.Buffer);
		
}


void handleDnsPacketByClient(CNdisApi &ndisApi,ETH_REQUEST &ethPacket,ULONG srcClientIp) {
	iphdr_ptr pIpHead   = (iphdr_ptr)((PUCHAR)ethPacket.EthPacket.Buffer->m_IBuffer + ETHER_HEADER_LENGTH);
	udphdr_ptr pUdpHead = (udphdr_ptr)((PUCHAR)pIpHead + (pIpHead->ip_hl * 32 / 8));
	if(!(isLocalIp(pIpHead->ip_dst.S_un.S_addr) && ntohs(pUdpHead->th_dport) == 53))return;
	if(dnsIp.S_un.S_addr == 0)return;

#ifdef _DEBUG
	CHAR sIpInfo[500] = { 0 };
	sprintf_s(sIpInfo, "%d.%d.%d.%d:%d --> %d.%d.%d.%d:%d [id=%d]",
		pIpHead->ip_src.S_un.S_un_b.s_b1,
		pIpHead->ip_src.S_un.S_un_b.s_b2,
		pIpHead->ip_src.S_un.S_un_b.s_b3,
		pIpHead->ip_src.S_un.S_un_b.s_b4,
		ntohs(pUdpHead->th_sport),
		pIpHead->ip_dst.S_un.S_un_b.s_b1,
		pIpHead->ip_dst.S_un.S_un_b.s_b2,
		pIpHead->ip_dst.S_un.S_un_b.s_b3,
		pIpHead->ip_dst.S_un.S_un_b.s_b4,
		ntohs(pUdpHead->th_dport),
		ntohs(pIpHead->ip_id));
	printf("[client]%s dns reiceive\n ", sIpInfo);
#endif

	//save original information
	ULONG dest_ip    =  dnsIp.S_un.S_addr;
	USHORT dest_port =  53;
	ULONG src_ip     =  pIpHead->ip_src.S_un.S_addr;
	USHORT src_port  =  ntohs(pUdpHead->th_sport);
	ULONG natIp      =  providerIp.S_un.S_addr;
	natEntry *natEntry = getByPutUdp(dest_ip, dest_port, src_ip, src_port, natIp,srcClientIp);

	pIpHead->ip_src.S_un.S_addr = natEntry->natIp;      //modify source ip address
	pUdpHead->th_sport = htons(natEntry->natPortOrId);  //modify source port address
	pIpHead->ip_dst.S_un.S_addr = dest_ip;              //modify dest ip address
		
	ndisApi.RecalculateUDPChecksum(ethPacket.EthPacket.Buffer);
	ndisApi.RecalculateIPChecksum(ethPacket.EthPacket.Buffer);
		
}

void handleIcmpPacketByClient(CNdisApi &ndisApi,ETH_REQUEST &ethPacket,ULONG srcClientIp) {
	iphdr_ptr pIpHead    = (iphdr_ptr)((PUCHAR)ethPacket.EthPacket.Buffer->m_IBuffer + ETHER_HEADER_LENGTH);
	icmphdr_ptr pIcmpHead = (icmphdr_ptr)((PUCHAR)pIpHead + (pIpHead->ip_hl * 32 / 8));

#ifdef _DEBUG
	CHAR sIpInfo[500] = { 0 };
	sprintf_s(sIpInfo, "%d.%d.%d.%d:%d --> %d.%d.%d.%d:%d [id=%d]",
		pIpHead->ip_src.S_un.S_un_b.s_b1,
		pIpHead->ip_src.S_un.S_un_b.s_b2,
		pIpHead->ip_src.S_un.S_un_b.s_b3,
		pIpHead->ip_src.S_un.S_un_b.s_b4,
		ntohs(pIcmpHead->id),
		pIpHead->ip_dst.S_un.S_un_b.s_b1,
		pIpHead->ip_dst.S_un.S_un_b.s_b2,
		pIpHead->ip_dst.S_un.S_un_b.s_b3,
		pIpHead->ip_dst.S_un.S_un_b.s_b4,
		ntohs(pIcmpHead->id),
		ntohs(pIpHead->ip_id));
	printf("[client]%s icmp reiceive\n ", sIpInfo);
#endif

	//save original information
	ULONG dest_ip    =  pIpHead->ip_dst.S_un.S_addr;
	ULONG src_ip     =  pIpHead->ip_src.S_un.S_addr;
	USHORT icmpId    =  ntohs(pIcmpHead->id);
	ULONG natIp      =  providerIp.S_un.S_addr;
	
	natEntry *natEntry = getByPutIcmp(dest_ip, src_ip, icmpId,natIp,srcClientIp);

	pIpHead->ip_src.S_un.S_addr = natEntry->natIp;  //modify source ip address
	pIcmpHead->id =  htons(natEntry->natPortOrId);  //modify id address
	ndisApi.RecalculateICMPChecksum (ethPacket.EthPacket.Buffer);
	ndisApi.RecalculateIPChecksum(ethPacket.EthPacket.Buffer);
		
}
