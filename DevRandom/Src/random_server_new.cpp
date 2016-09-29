#include "stdafx.h"

#ifdef _DEBUG
#define new DEBUG_NEW

#endif


#include "IThreadPool.h"
#include "IWork.h"
#include "ServiceManager.h"
#include "RandomDataPipeServer.h"

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
			int retVal = RunServer2();

			_tprintf_s(TEXT("Shutdown Complete. Press Enter to exit...\n> "));
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

static void WaitForCallbacks(IDevRandomServer::Ptr &pServer)
{
	TCHAR buf[32] = {0};
	for(;;)
	{
		_tprintf_s(TEXT("Enter \"quit\" to exit\n> "));
		LPCTSTR lpsz = _getws_s(buf);
        if (lpsz)
        {
            if (0 == _tcsicmp(lpsz, TEXT("quit")))
                break;
            else if (0 == _tcsicmp(lpsz, TEXT("check")))
                pServer->CheckHandle();
        }
	}
}


int RunServer2()
{
	IThreadPool::Ptr pPool = IThreadPool::newPool(IThreadPool::NORMAL,0,0);
	if( pPool )
	{
		_CrtMemState state;
		UNREFERENCED_PARAMETER(state);
		_CrtMemCheckpoint(&state);
		{
			IDevRandomServer::Ptr pServer = createPipeServer(TEXT("\\\\.\\pipe\\random"), pPool.get());
			if( pServer )
			{
				if( pServer->runServer() )
				{
					WaitForCallbacks(pServer);
					pServer->shutDownServer();
				}
			}
		}
		_CrtMemDumpAllObjectsSince(&state);
	}	
	
	return 0;
}

