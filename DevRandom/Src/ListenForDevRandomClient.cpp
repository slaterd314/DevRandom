#include "stdafx.h"
#include "ListenForDevRandomClient.h"
#include "DevRandomClientConnection.h"


ListenForDevRandomClient::ListenForDevRandomClient(LPCTSTR lpszPipeName, HANDLE hStopEvent, IThreadPool *pPool) : 
m_hPipe(INVALID_HANDLE_VALUE), 
m_pipeName(lpszPipeName), 
m_hStopEvent(hStopEvent),
m_stop(false)
{
	TRACE(TEXT(">>>ListenForDevRandomClient::ListenForDevRandomClient(), this=0x%x.\n"), this); 
	m_work = pPool->newWork([&](PTP_CALLBACK_INSTANCE Instance, IWork *pWork) {
		listenForClient(Instance, pWork);
	});

	if( !m_work )
		throw ::std::runtime_error("ListenForDevRandomClient::ListenForDevRandomClient() - newWork() failed");
}

ListenForDevRandomClient::~ListenForDevRandomClient()
{
	TRACE(TEXT("<<<ListenForDevRandomClient::~ListenForDevRandomClient(), this=0x%x.\n"), this); 
}

bool
ListenForDevRandomClient::runServer()
{
	bool bRetVal = false;
	if( m_work )
		bRetVal = m_work->SubmitWork();
	return bRetVal;
}

void 
ListenForDevRandomClient::shutDownServer()
{
	m_stop = true;
	::SetEvent(m_hStopEvent);
	DevRandomClientConnection::waitForClientsToStop();
	CloseHandle(m_hStopEvent);
	if( INVALID_HANDLE_VALUE != m_hPipe )
	{
		CancelIoEx(m_hPipe, m_olp.get());
		m_hPipe = INVALID_HANDLE_VALUE;
		// m_pio->WaitForCallbacks(TRUE);
		m_work->WaitForCallbacks(TRUE);
		// m_pio->CloseIo();
		m_work->CloseWork();
		CloseHandle(m_hPipe);
	}
}

void
ListenForDevRandomClient::makeSelfReferent()
{
	// self-reference
	m_self = shared_from_this();
}

ListenForDevRandomClient::Ptr
ListenForDevRandomClient::create(LPCTSTR lpszPipeName, HANDLE hStopEvent, IThreadPool *pPool)
{
#ifdef _DEBUG
	return ::std::shared_ptr<ListenForDevRandomClient>(new ListenForDevRandomClient(lpszPipeName, hStopEvent, pPool));
#else
	return ::std::make_shared<ListenForDevRandomClient>(lpszPipeName, hStopEvent, pPool);
#endif
}

bool
ListenForDevRandomClient::startServer()
{
	bool bRetVal = false;
	if( m_work  )
		bRetVal = m_work->SubmitWork();
	return bRetVal;
}

void
ListenForDevRandomClient::CheckHandle()
{
	if( INVALID_HANDLE_VALUE != m_hPipe )
	{
		DWORD dwState = 0;
		DWORD dwCurInstance=0;
		DWORD dwMaxCollectionCount = 0;
		DWORD dwCollectionDataTimeout = 0;
		// TCHAR UserName[1024];

		BOOL bRet = GetNamedPipeHandleState(m_hPipe,&dwState, &dwCurInstance, NULL /* &dwMaxCollectionCount */ , NULL /* &dwCollectionDataTimeout */, NULL, 0);
		if( !bRet )
		{
			_ftprintf_s(stderr,TEXT("GetNamedPipeHandleState failed, GLE=%d.\n"), GetLastError()); 
		}
		else
		{
			_ftprintf_s(stderr,TEXT("GetNamedPipeHandleState returned:\nState:%d\nCurInstances:%d\nMaxCollectionCount:%d\nCollectionDataTimeout:%d\nUserName:%s\n"),
				dwState, dwCurInstance, dwMaxCollectionCount, dwCollectionDataTimeout, TEXT(""));
		}

		DWORD dwFlags = 0;
		DWORD dwOutBufSize=0;
		DWORD dwInBufSize = 0;
		DWORD dwMaxInstances = 0;

		bRet = GetNamedPipeInfo(m_hPipe, &dwFlags, &dwOutBufSize, &dwInBufSize, &dwMaxInstances);

		if( bRet )
		{
			_ftprintf_s(stderr,TEXT("GetNamedPipeInfo returned:\nFlags:%d\nOutBufferSize:%d\nInBufferSize:%d\nMaxInstances:%d\n"),
				dwFlags, dwOutBufSize, dwInBufSize, dwMaxInstances);
		}
		else
		{
			_ftprintf_s(stderr,TEXT("GetNamedPipeInfo failed, GLE=%d.\n"), GetLastError()); 
		}

		DWORD dwNumBytes = 0;
		if( GetOverlappedResult(m_hPipe, m_olp.get(), &dwNumBytes, FALSE) )
		{
			_ftprintf_s(stderr,TEXT("GetOverlappedResult returned TRUE.\n"));
		}
		else if( ERROR_IO_PENDING == GetLastError() )
		{
			;
		}

	}
}

void
ListenForDevRandomClient::listenForClient(PTP_CALLBACK_INSTANCE , IWork *pWork)
{
	TRACE(TEXT(">>>ListenForDevRandomClient::listenForClient() enter\n")); 
	if(!m_stop && pWork && pWork->pool() )
	{
		IThreadPool *pPool = pWork->pool();
			
		m_hPipe = CreateNamedPipe(m_pipeName.c_str(),
						PIPE_ACCESS_DUPLEX|FILE_FLAG_OVERLAPPED,
						PIPE_TYPE_MESSAGE|PIPE_WAIT,
						PIPE_UNLIMITED_INSTANCES,
						MyOverlapped::BUFSIZE,
						0,
						0,
						NULL);
		if( INVALID_HANDLE_VALUE == m_hPipe )
		{
			_ftprintf_s(stderr,TEXT("CreateNamedPipe failed, GLE=%d.\n"), GetLastError()); 
		}
		else
		{
			m_olp.reset(new MyOverlapped);

			m_olp->hEvent = ::CreateEvent(NULL,TRUE,FALSE,NULL);
			if( m_olp->hEvent )
			{
				if( RtlGenRandom(m_olp->buffer, MyOverlapped::BUFSIZE) )
				{
					m_wait = pPool->newWait([&](PTP_CALLBACK_INSTANCE Instance, TP_WAIT_RESULT Result, IWait *pWait){
						onConnectEventSignaled(Instance, Result, pWait);
					});
					
					if( m_wait )
					{
						m_wait->SetThreadpoolWait(m_olp->hEvent,NULL);
						BOOL fConnected = ConnectNamedPipe(m_hPipe, m_olp.get());	// asynchronous ConnectNamedPipe() call - this call will return immediately
																			// and the Io completion routine will be called when a client connects.
						if( !fConnected )
						{
							if( GetLastError() == ERROR_IO_PENDING || GetLastError() == ERROR_PIPE_CONNECTED )
								fConnected = TRUE;
						}
						if( !fConnected )
						{
							_ftprintf_s(stderr, TEXT("ConnectNamedPipe failed. GLE=%d\n"), GetLastError());
							m_wait->WaitForCallbacks(TRUE);
						}
					}
				}
				else
					_ftprintf_s(stderr, TEXT("RtlGenRandom failed. GLE=%d\n"), GetLastError());
			}
			else
				_ftprintf_s(stderr, TEXT("CreateEvent failed. GLE=%d\n"), GetLastError());
		}
	}
	TRACE(TEXT("<<<ListenForDevRandomClient::listenForClient() exit\n")); 
}

void
ListenForDevRandomClient::onConnectEventSignaled(PTP_CALLBACK_INSTANCE /*Instance*/, TP_WAIT_RESULT /*Result*/, IWait *pWait)
{
	TRACE(TEXT(">>>ListenForDevRandomClient::onConnectEventSignaled() enter\n"));
	IThreadPool *pPool = NULL;
	bool bReSubmit = false;

	if( !m_stop )
	{
		if( pWait )
		{
			pPool = pWait->pool();
			if( pPool )
			{
				MyOverlappedPtr olp(m_olp);
				bReSubmit = true;
				DevRandomClientConnection::Ptr pClient = DevRandomClientConnection::create(m_hPipe, olp, m_hStopEvent, pPool);

				if( pClient )
				{
					pClient->makeSelfReferent();
					if( !pClient->runClient() )
						pClient.reset();
				}
			}
			else
				_ftprintf_s(stderr, TEXT("ListenForDevRandomClient::onConnectEventSignaled pPool == NULL \n"));
		}
		else
			_ftprintf_s(stderr, TEXT("ListenForDevRandomClient::onConnectEventSignaled pWait == NULL \n"));

		TRACE(TEXT(">>>ListenForDevRandomClient::onConnectClient() exit\n")); 
		if( pPool && bReSubmit )
		{
			if( !m_work->SubmitWork() )
			{
				_ftprintf_s(stderr, TEXT("ListenForDevRandomClient::onConnectClient SubmitThreadpoolWork failed. GLE=%d\n"), GetLastError());
				{
					m_self.reset();
					return;
				}
			}
		}

	}
}

