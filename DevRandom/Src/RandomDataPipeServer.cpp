#include "stdafx.h"
#include "MyOverlapped.h"
#include "IIoCompletion.h"
#include "IThreadPool.h"
#include "IWork.h"
#include "SpinLock.h"
#include "RandomDataPipeServer.h"
#include "DevRandomClientConnection.h"
#include "ListenForDevRandomClient.h"

#ifdef _DEBUG
#define new DEBUG_NEW

#endif


IDevRandomServer::Ptr createPipeServer(LPCTSTR lpszPipeName, IThreadPool *pPool)
{
	IDevRandomServer::Ptr pServer;
	if( pPool )
	{
		HANDLE hStopEvent = ::CreateEvent(NULL,TRUE,FALSE,NULL);
		if( hStopEvent )
			pServer = ListenForDevRandomClient::create(lpszPipeName, hStopEvent, pPool);
	}
	return pServer;
}
