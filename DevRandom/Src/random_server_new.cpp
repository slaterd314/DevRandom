#include "stdafx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#define delete DEBUG_DELETE
#endif


#include "IThreadPool.h"
#include "IClientConnection.h"
#include "IWork.h"
#include "ServiceManager.h"

static int RunServer2();


int _tmain(int argc, TCHAR **argv)
{
	if( argc == 1 )
	{
		ServiceManager::RunService();
	}
	else
	{
		LPCTSTR lpszCommand = argv[1];
		if( 0 == _tcsicmp(lpszCommand, TEXT("-install")) )
		{
			ServiceManager::SvcInstall();
		}
		else if( 0 == _tcsicmp(lpszCommand, TEXT("-remove")) )
		{
			ServiceManager::SvcRemove();
		}
		else if( 0 == _tcsicmp(lpszCommand, TEXT("-d")) )
		{
			_CrtMemState state;
			UNREFERENCED_PARAMETER(state);
			_CrtMemCheckpoint(&state);
			int retVal = RunServer2();
			_CrtMemDumpAllObjectsSince(&state);

			_ftprintf_s(stdout, TEXT("Shutdown Complete. Press Enter to exit...\n> "));
			TCHAR buffer[32];
			_getws_s(buffer);

			return retVal;
		}
		else
		{
			_ftprintf_s(stderr, TEXT("Error: unrecognized switch %s\n"), lpszCommand);
			return 1;
		}
	}
	return 0;
}

static void WaitForCallbacks()
{
	TCHAR buf[32] = {0};
	for(;;)
	{
		_ftprintf_s(stdout, TEXT("Enter \"quit\" to exit\n> "));
		LPCTSTR lpsz = _getws_s(buf);
		if( 0 == _tcsicmp(lpsz, TEXT("quit")) )
			break;
	}
}


int RunServer2()
{
	IThreadPoolPtr pPool = createThreadPool(IThreadPool::LOW);
	if( pPool.get() )
	{
		IClientConnection *pConn = pPool->newNamedPipeConnection();
		if( pConn )
		{
			IWork *pWork = pPool->newWaitForNewConnection(pConn);
			if( pWork )
				pPool->SubmitThreadpoolWork(pWork);
		}		
	}	
	WaitForCallbacks();
	pPool->Shutdown();
	pPool.reset();
	
	return 0;
}
