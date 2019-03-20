#include "stdafx.h"
#include "fileprint.h"

void fileprint(const char* msg){
	/*
	HANDLE hFile = ::CreateFile("c:\\test.txt",
								GENERIC_READ | GENERIC_WRITE,
								FILE_SHARE_READ | FILE_SHARE_WRITE,
								NULL,
								OPEN_EXISTING ,
								FILE_FLAG_WRITE_THROUGH | FILE_ATTRIBUTE_NORMAL,
								NULL);
	DWORD ret = 0;
	::SetFilePointer(hFile,0,NULL,FILE_END);
	::WriteFile(hFile,msg,::strlen(msg),&ret,NULL);
	::WriteFile(hFile,"\r\n",2,&ret,NULL);
	::CloseHandle(hFile);
	printf("ret = %d \n " ,ret);
	*/
}

