// ServiceTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "ServiceMain.h"
#include "NatAdapter.h"
#include <string>
#include <windows.h>  
#include <tchar.h>  
#include <strsafe.h> 



#define FACILITY_SYSTEM                  0x0
#define FACILITY_RUNTIME                 0x2
#define FACILITY_STUBS                   0x3
#define FACILITY_IO_ERROR_CODE           0x4

//
// Define the severity codes
//
#define STATUS_SEVERITY_SUCCESS          0x0
#define STATUS_SEVERITY_INFORMATIONAL    0x1
#define STATUS_SEVERITY_WARNING          0x2
#define STATUS_SEVERITY_ERROR            0x3
#define SVC_ERROR                        ((DWORD)0xC0020001L)


SERVICE_STATUS          gSvcStatus;
SERVICE_STATUS_HANDLE   gSvcStatusHandle;
HANDLE                  ghSvcStopEvent = NULL;


void WINAPI SvcCtrlHandler(DWORD);
void WINAPI SvcMain(DWORD, LPTSTR *);

void ReportSvcStatus(DWORD, DWORD, DWORD);
void SvcInit(DWORD, LPTSTR *);
void SvcReportEvent(LPTSTR);

DWORD WINAPI smThreadRun(LPVOID lpThreadParameter);

pMain pNatMain = NULL;
pStop pNatMainStop = NULL;

//  
// Purpose:   
//   Entry point for the process  
//  
// Parameters:  
//   None  
//   
// Return value:  
//   None  
//  
int __cdecl serviceMain(int argc, TCHAR *argv[])
{
	/*
	// If command-line parameter is "install", install the service.   
	// Otherwise, the service is probably being started by the SCM.  

	if (lstrcmpi(argv[1], TEXT("install")) == 0)
	{
		printf_s("install service .... \n ");
		SvcInstall();
		printf_s("install service over \n");
		return 0;
	}
	
	// TO_DO: Add any additional services for the process to this table.  
	SERVICE_TABLE_ENTRY DispatchTable[] =
	{
		{ (wchar_t*)SVCNAME, (LPSERVICE_MAIN_FUNCTION)SvcMain },
		{ NULL, NULL }
	};

	// This call returns when the service has stopped.   
	// The process should simply terminate when the call returns.  

	if (!StartServiceCtrlDispatcher(DispatchTable))
	{
		SvcReportEvent((wchar_t*)"StartServiceCtrlDispatcher");
	}
	printf_s("over");
	int d = 0;
	scanf_s("%d", &d);
	*/
	
	printf_s("\nexist main");
	return 0;
}

//  
// Purpose:   
//   Installs a service in the SCM database  
//  
// Parameters:  
//   None  
//   
// Return value:  
//   None  
//  
void SvcInstall()
{
	SC_HANDLE schSCManager;
	SC_HANDLE schService;
	TCHAR szPath[MAX_PATH] = {0};

	if (!GetModuleFileName(NULL, szPath, MAX_PATH))
	{
		printf("Cannot install service (%d)\n", GetLastError());
		return;
	}
	StringCbCat(szPath,MAX_PATH," service");
	if(::strlen(clientMac)>0){
		StringCbCat(szPath,MAX_PATH," ");
		StringCbCat(szPath,MAX_PATH,clientMac);
	}
	// Get a handle to the SCM database.   

	schSCManager = OpenSCManager(
		NULL,                    // local computer  
		NULL,                    // ServicesActive database   
		SC_MANAGER_ALL_ACCESS);  // full access rights   

	if (NULL == schSCManager)
	{
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return;
	}

	// Delete the service
	schService = OpenService(schSCManager,SVCNAME,DELETE);
	if(DeleteService(schService)){
		printf("delete %s success \n ",SVCNAME);
	}


	// Create the service  

	schService = CreateService(
		schSCManager,              // SCM database   
		SVCNAME,                   // name of service   
		SVCNAME,                   // service name to display   
		SERVICE_ALL_ACCESS,        // desired access   
		SERVICE_WIN32_OWN_PROCESS, // service type   
		SERVICE_AUTO_START,        // start type   
		SERVICE_ERROR_NORMAL,      // error control type   
		szPath,                    // path to service's binary   
		NULL,                      // no load ordering group   
		NULL,                      // no tag identifier   
		"Dnscache",                // no dependencies   
		NULL,                      // LocalSystem account   
		NULL);                     // no password   

	if (schService == NULL)
	{
		printf("CreateService failed (%d)\n", GetLastError());
		CloseServiceHandle(schSCManager);
		return;
	}
	else printf("Service installed successfully\n");

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
}

//  
// Purpose:   
//   Installs a service in the SCM database  
//  
// Parameters:  
//   None  
//   
// Return value:  
//   None  
//  
void SvcExecute(pMain startHandle,pStop stopHandle){
	pNatMain = startHandle;
	pNatMainStop = stopHandle;
	SERVICE_TABLE_ENTRY DispatchTable[] =
	{
		{ SVCNAME, (LPSERVICE_MAIN_FUNCTION)SvcMain },
		{ NULL, NULL }
	};

	// This call returns when the service has stopped.   
	// The process should simply terminate when the call returns.  

	if (!StartServiceCtrlDispatcher(DispatchTable))
	{
		SvcReportEvent("StartServiceCtrlDispatcher");
	}
}


//  
// Purpose:   
//   Entry point for the service  
//  
// Parameters:  
//   dwArgc - Number of arguments in the lpszArgv array  
//   lpszArgv - Array of strings. The first string is the name of  
//     the service and subsequent strings are passed by the process  
//     that called the StartService function to start the service.  
//   
// Return value:  
//   None.  
//  
void WINAPI SvcMain(DWORD dwArgc, LPTSTR *lpszArgv)
{

	std::string msg;
	char m[200] = {0};
	sprintf_s(m,200,"SvcMain dwArgc = %d ",dwArgc);
	msg += m;
	for(int i=0;i<dwArgc;i++){
		char o[200] = {0};
		sprintf_s(o,200,"lpszArgv[%d] = %s ",i,lpszArgv[i]);
		msg += o;
	}

	// Register the handler function for the service  

	gSvcStatusHandle = RegisterServiceCtrlHandler(SVCNAME,SvcCtrlHandler);

	if (!gSvcStatusHandle)	{
		//LPSTR a = "RegisterServiceCtrlHandler";
		SvcReportEvent("RegisterServiceCtrlHandler");
		return;
	}

	// These SERVICE_STATUS members remain as set here  

	gSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	gSvcStatus.dwServiceSpecificExitCode = 0;

	// Report initial status to the SCM  

	ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

	// Perform service-specific initialization and work.  
	
	SvcInit(dwArgc, lpszArgv);
}

//  
// Purpose:   
//   The service code  
//  
// Parameters:  
//   dwArgc - Number of arguments in the lpszArgv array  
//   lpszArgv - Array of strings. The first string is the name of  
//     the service and subsequent strings are passed by the process  
//     that called the StartService function to start the service.  
//   
// Return value:  
//   None  
//  
void SvcInit(DWORD dwArgc, LPTSTR *lpszArgv)
{
	std::string msg;
	char m[200] = {0};
	sprintf_s(m,200,"SvcInit dwArgc = %d ",dwArgc);
	msg += m;
	for(int i=0;i<dwArgc;i++){
		char o[200] = {0};
		sprintf_s(o,200,"lpszArgv[%d] = %s ",i,lpszArgv[i]);
		msg += o;
	}

	// TO_DO: Declare and set any required variables.  
	//   Be sure to periodically call ReportSvcStatus() with   
	//   SERVICE_START_PENDING. If initialization fails, call  
	//   ReportSvcStatus with SERVICE_STOPPED.  

	// Create an event. The control handler function, SvcCtrlHandler,  
	// signals this event when it receives the stop control code.  

	ghSvcStopEvent = CreateEvent(
		NULL,    // default security attributes  
		TRUE,    // manual reset event  
		FALSE,   // not signaled  
		NULL);   // no name  

	if (ghSvcStopEvent == NULL)
	{
		ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
		return;
	}

	// Report running status when initialization is complete.  
	ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);
	if(pNatMain != NULL)::CreateThread(NULL, 0, smThreadRun, NULL, 0, NULL);

	// TO_DO: Perform work until service stops.  
	
	while (1)
	{
		// Check whether to stop the service.  

		WaitForSingleObject(ghSvcStopEvent, INFINITE);
		if(pNatMainStop!=NULL)pNatMainStop();
		ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
		return;
	}
	
}

//  
// Purpose:   
//   Sets the current service status and reports it to the SCM.  
//  
// Parameters:  
//   dwCurrentState - The current state (see SERVICE_STATUS)  
//   dwWin32ExitCode - The system error code  
//   dwWaitHint - Estimated time for pending operation,   
//     in milliseconds  
//   
// Return value:  
//   None  
//  
void ReportSvcStatus(DWORD dwCurrentState,
	DWORD dwWin32ExitCode,
	DWORD dwWaitHint)
{
	static DWORD dwCheckPoint = 1;

	// Fill in the SERVICE_STATUS structure.  

	gSvcStatus.dwCurrentState = dwCurrentState;
	gSvcStatus.dwWin32ExitCode = dwWin32ExitCode;
	gSvcStatus.dwWaitHint = dwWaitHint;

	if (dwCurrentState == SERVICE_START_PENDING)
		gSvcStatus.dwControlsAccepted = 0;
	else gSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

	if ((dwCurrentState == SERVICE_RUNNING) ||
		(dwCurrentState == SERVICE_STOPPED))
		gSvcStatus.dwCheckPoint = 0;
	else gSvcStatus.dwCheckPoint = dwCheckPoint++;

	// Report the status of the service to the SCM.  
	SetServiceStatus(gSvcStatusHandle, &gSvcStatus);
}

//  
// Purpose:   
//   Called by SCM whenever a control code is sent to the service  
//   using the ControlService function.  
//  
// Parameters:  
//   dwCtrl - control code  
//   
// Return value:  
//   None  
//  
void WINAPI SvcCtrlHandler(DWORD dwCtrl)
{
	// Handle the requested control code.   

	switch (dwCtrl)	{

	case SERVICE_CONTROL_STOP:
		ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);

		// Signal the service to stop.  

		SetEvent(ghSvcStopEvent);
		ReportSvcStatus(gSvcStatus.dwCurrentState, NO_ERROR, 0);

		return;

	case SERVICE_CONTROL_INTERROGATE:
		break;

	default:
		break;
	}

}

//  
// Purpose:   
//   Logs messages to the event log  
//  
// Parameters:  
//   szFunction - name of function that failed  
//   
// Return value:  
//   None  
//  
// Remarks:  
//   The service must have an entry in the Application event log.  
//  
void SvcReportEvent(LPTSTR szFunction)
{
	HANDLE hEventSource;
	LPCTSTR lpszStrings[2];
	TCHAR Buffer[80];

	hEventSource = RegisterEventSource(NULL, SVCNAME);

	if (NULL != hEventSource)
	{
		StringCchPrintf(Buffer, 80, TEXT("%s failed with %d"), szFunction, GetLastError());

		lpszStrings[0] = SVCNAME;
		lpszStrings[1] = Buffer;

		ReportEvent(hEventSource,        // event log handle  
			EVENTLOG_ERROR_TYPE, // event type  
			0,                   // event category  
			SVC_ERROR,           // event identifier  
			NULL,                // no security identifier  
			2,                   // size of lpszStrings array  
			0,                   // no binary data  
			lpszStrings,         // array of strings  
			NULL);               // no binary data  

		DeregisterEventSource(hEventSource);
	}
}



DWORD WINAPI smThreadRun(LPVOID lpThreadParameter){
	pNatMain(NULL);
	return 0;
}