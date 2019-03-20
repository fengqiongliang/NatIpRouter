
#include "stdafx.h"
#include "iphlp.h"
#include <string>
#include "NatAdapter.h"
#include "NatMain.h";
#include "ServiceMain.h"
#include "fileprint.h"


using namespace std;
int _tmain(int argc, _TCHAR* argv[])
{
	printf("NatIpRouter.exe install 00-0C-29-CE-3C-43\n");
	printf("NatIpRouter.exe install\n");
	printf("NatIpRouter.exe 00-0C-29-CE-3C-43\n");
	printf("NatIpRouter.exe\n");
	std::string msg;
	char m[200] = {0};
	sprintf_s(m,200,"argc = %d ",argc);
	msg += m;
	for(int i=0;i<argc;i++){
		char o[200] = {0};
		sprintf_s(o,200,"argc[%d] = %s ",i,argv[i]);
		msg += o;
	}
	//start from service
	if(argc == 2 && ::_strcmpi(argv[1],"service") == 0){
		ZeroMemory(clientMac,macLenth);
		SvcExecute(natMain,natStop);
		return 0;
	}
	if(argc == 3 && ::_strcmpi(argv[1],"service") == 0){
		ZeroMemory(clientMac,macLenth);
		::strcpy_s(clientMac, macLenth, argv[2]);
		SvcExecute(natMain,natStop);
		return 0;
	}
	//start from console and install service
	if(argc == 2 && ::_strcmpi(argv[1],"install") == 0){
		ZeroMemory(clientMac,macLenth);
		SvcInstall();
		return 0;
	}
	if(argc == 3 && ::_strcmpi(argv[1],"install") == 0){
		ZeroMemory(clientMac,macLenth);
		::strcpy_s(clientMac, macLenth, argv[2]);
		SvcInstall();
		return 0;
	}
	//start from console
	if(argc == 2){
		natMain(argv[1]);
		return 0;
	}
	natMain(NULL);
	return 0;
}


