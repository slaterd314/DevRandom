#include "stdafx.h"

#ifdef _DEBUG
#define new DEBUG_NEW

#endif


#include "ServiceManager.h"
#include "IWork.h"
#include "RandomDataPipeServer.h"


#ifdef ENABLE_SERVICE

LPCTSTR ServiceManager::lpszServiceName = TEXT("DevRandom");
LPCTSTR ServiceManager::lpszServiceDescription = TEXT("Random Binary Data Generator");
SERVICE_STATUS          ServiceManager::gSvcStatus;
SERVICE_STATUS_HANDLE   ServiceManager::gSvcStatusHandle = NULL;
IThreadPoolPtr			ServiceManager::pPool;
IDevRandomServer::Ptr	ServiceManager::m_Server;
HANDLE					ServiceManager::m_hEvent=NULL;

// static
VOID
ServiceManager::RunService()
{

	LPTSTR lpsz = _tcsdup(lpszServiceName);
	// TO_DO: Add any additional services for the process to this table.
	SERVICE_TABLE_ENTRY DispatchTable[] = 
	{ 
		{lpsz , (LPSERVICE_MAIN_FUNCTION) SvcMain }, 
		{ NULL, NULL } 
	}; 
	if (!StartServiceCtrlDispatcher( DispatchTable )) 
	{ 
		SvcReportEvent(TEXT("StartServiceCtrlDispatcher")); 
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
VOID
ServiceManager::ReportSvcStatus( DWORD dwCurrentState,
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
  
	if ( (dwCurrentState == SERVICE_RUNNING) || (dwCurrentState == SERVICE_STOPPED) )
		gSvcStatus.dwCheckPoint = 0;
	else gSvcStatus.dwCheckPoint = dwCheckPoint++;
  
	// Report the status of the service to the SCM.
	SetServiceStatus( gSvcStatusHandle, &gSvcStatus );
}


DWORD
WINAPI
ServiceManager::HandlerEx(__in  DWORD dwControl, __in  DWORD /*dwEventType*/, __in  LPVOID /*lpEventData*/, __in  LPVOID /*lpContext*/ )
{
	DWORD dwRetVal = NO_ERROR;
	switch( dwControl )
	{
		case SERVICE_CONTROL_STOP:
			ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
			::SetEvent(m_hEvent);
			pPool->Shutdown();
			m_Server.reset();
			CloseHandle(m_hEvent);
			ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
			break;
		case SERVICE_CONTROL_INTERROGATE:
		default:
			break;
	}
	return dwRetVal;
}

VOID
WINAPI
ServiceManager::SvcMain( DWORD dwArgc, LPTSTR *lpszArgv )
{

	// Register the handler function for the service
  
    gSvcStatusHandle = RegisterServiceCtrlHandlerEx( lpszServiceName, HandlerEx, NULL);
  
    if( !gSvcStatusHandle )
    { 
        SvcReportEvent(TEXT("RegisterServiceCtrlHandlerEx")); 
        return; 
    } 
  
    // These SERVICE_STATUS members remain as set here
  
    gSvcStatus.dwServiceType = SERVICE_WIN32_SHARE_PROCESS; 
    gSvcStatus.dwServiceSpecificExitCode = 0;    
  
    // Report initial status to the SCM
  
    ReportSvcStatus( SERVICE_START_PENDING, NO_ERROR, 3000 );
  
    // Perform service-specific initialization and work.
  
    SvcInit( dwArgc, lpszArgv );
	
}

VOID
ServiceManager::SvcInit( DWORD, LPTSTR * )
{
	pPool = IThreadPool::newPool(IThreadPool::LOW);
	if( pPool )
	{
		m_hEvent = ::CreateEvent(NULL,TRUE,FALSE,NULL);
		if( m_hEvent )
		{
			m_Server = createPipeServer(TEXT("\\\\.\\pipe\\random"), m_hEvent, pPool.get());
			if( m_Server && m_Server->runServer() )
			{
				ReportSvcStatus( SERVICE_RUNNING, NO_ERROR, 0 );
			}
			else
				ReportSvcStatus( SERVICE_STOPPED, ERROR_SERVICE_SPECIFIC_ERROR, 0 );
		}
		else
			ReportSvcStatus( SERVICE_STOPPED, ERROR_INVALID_HANDLE, 0 );
	}
	else
		ReportSvcStatus( SERVICE_STOPPED, ERROR_NOT_ENOUGH_MEMORY, 0 );
	//IWorkPtr pWork = makePipeServer(TEXT("\\\\.\\pipe\\random"), pPool.get());
	//if( pWork )
	//{
	//	pPool->SubmitThreadpoolWork(pWork.get());
	//}

#ifdef DEPRECATED
	if( pPool )
	{
		IClientConnection *pConn = pPool->newNamedPipeConnection();
		if( pConn )
		{
			IWork *pWork = pPool->newWaitForNewConnection(pConn);
			if( pWork )
			{
				pPool->SubmitThreadpoolWork(pWork);
				ReportSvcStatus( SERVICE_RUNNING, NO_ERROR, 0 );
			}
		}		
	}	
#endif // DEPRECATED
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
VOID
ServiceManager::SvcReportEvent(LPTSTR szFunction) 
{ 
    HANDLE hEventSource;
    LPCTSTR lpszStrings[2];
    TCHAR Buffer[80];
  
    hEventSource = RegisterEventSource(NULL, lpszServiceName);
  
    if( NULL != hEventSource )
    {
        StringCchPrintf(Buffer, 80, TEXT("%s failed with %d"), szFunction, GetLastError());
  
        lpszStrings[0] = lpszServiceName;
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
VOID
ServiceManager::SvcInstall()
{
    SC_HANDLE schSCManager;
    SC_HANDLE schService;
    TCHAR szPath[MAX_PATH];
  
    if( !GetModuleFileName( NULL, szPath, MAX_PATH ) )
    {
        _ftprintf_s(stderr,TEXT("Cannot install service (%d)\n"), GetLastError());
        return;
    }
  
    // Get a handle to the SCM database. 
   
    schSCManager = OpenSCManager( 
        NULL,                    // local computer
        NULL,                    // ServicesActive database 
        SC_MANAGER_ALL_ACCESS);  // full access rights 
   
    if (NULL == schSCManager) 
    {
        _ftprintf_s(stderr,TEXT("OpenSCManager failed (%d)\n"), GetLastError());
        return;
    }
  
    // Create the service
  
	schService = CreateService( 
        schSCManager,             // SCM database 
        lpszServiceName,			// name of service 
        lpszServiceDescription,	  // service name to display 
        SERVICE_ALL_ACCESS,       // desired access 
        SERVICE_WIN32_OWN_PROCESS,// service type 
        SERVICE_DEMAND_START,     // start type 
        SERVICE_ERROR_NORMAL,     // error control type 
        szPath,                   // path to service's binary 
        NULL,                     // no load ordering group 
        NULL,                     // no tag identifier 
        NULL,                     // no dependencies 
        TEXT("NT AUTHORITY\\LocalService"), // LocalSystem account 
        NULL);                    // no password 
   
	if (schService == NULL) 
	{
		_ftprintf_s(stderr,TEXT("CreateService failed (%d)\n"), GetLastError()); 
		CloseServiceHandle(schSCManager);
		return;
	}
	else
	{
		_ftprintf_s(stdout,TEXT("Service installed successfully\n")); 
		SERVICE_DESCRIPTION desc = { TEXT("Random data generator that acts like a unix /dev/random") };
		ChangeServiceConfig2(schSCManager, SERVICE_CONFIG_DESCRIPTION, &desc);
	}
  
    CloseServiceHandle(schService); 
    CloseServiceHandle(schSCManager);
}

VOID
ServiceManager::SvcRemove()
{
    SC_HANDLE schSCManager = OpenSCManager( 
								NULL,                    // local computer
								NULL,                    // ServicesActive database 
								SC_MANAGER_ALL_ACCESS);  // full access rights 
   
	BOOL bResult = FALSE;
	if (NULL == schSCManager) 
	{
		_ftprintf_s(stderr, TEXT("OpenSCManager failed (%d)\n"), GetLastError());
		return;
	}
	SC_HANDLE hService = OpenService(schSCManager, lpszServiceName, SERVICE_ALL_ACCESS);
	if( !hService)
	{
		_ftprintf_s(stderr, TEXT("OpenService failed (%d)\n"), GetLastError());
	}
	else
	{
		bResult = DeleteService(hService);
		CloseServiceHandle(hService);
	}
	if(!bResult)
	{
		_ftprintf_s(stderr, TEXT("DeleteService failed (%d)\n"), GetLastError());
	}
	else
		_ftprintf_s(stdout, TEXT("Service removed successfully\n") );
}
  
#endif // ENABLE_SERVICE
  
  