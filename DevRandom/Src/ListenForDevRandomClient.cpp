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
}

ListenForDevRandomClient::~ListenForDevRandomClient()
{
	TRACE(TEXT("<<<ListenForDevRandomClient::~ListenForDevRandomClient(), this=0x%x.\n"), this); 
}

bool
ListenForDevRandomClient::runServer()
{
	bool bRetVal = false;
	if( m_work && m_work->pool() )
		bRetVal = m_work->pool()->SubmitThreadpoolWork(m_work.get());
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
		m_pio->pool()->WaitForThreadpoolIoCallbacks(m_pio.get(), TRUE);
		m_work->pool()->WaitForThreadpoolWorkCallbacks(m_work.get(), TRUE);
		m_pio->pool()->CloseThreadpoolIo(m_pio.get());
		m_work->pool()->CloseThreadpoolWork(m_work.get());
	}
}

void
ListenForDevRandomClient::makeSelfReference()
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
	if( m_work && m_work->pool() )
		bRetVal = m_work->pool()->SubmitThreadpoolWork(m_work.get());
	return bRetVal;
}

void
ListenForDevRandomClient::listenForClient(PTP_CALLBACK_INSTANCE , IWork *pWork)
{
	TRACE(TEXT(">>>ListenForDevRandomClient::listenForClient() enter\n")); 
	if(!m_stop && pWork && pWork->pool() )
	{
		IThreadPool *pPool = pWork->pool();
			
		m_hPipe = CreateNamedPipe(m_pipeName.c_str(),
						PIPE_ACCESS_OUTBOUND|FILE_FLAG_OVERLAPPED,
						PIPE_TYPE_MESSAGE|PIPE_WAIT,
						PIPE_UNLIMITED_INSTANCES,
						MyOverlapped::BUFSIZE,
						MyOverlapped::BUFSIZE,
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
					m_pio = pPool->newIoCompletion(m_hPipe, [&](PTP_CALLBACK_INSTANCE Instance, PVOID Overlapped, ULONG IoResult, ULONG_PTR nBytesTransfered, IIoCompletion *pIo){
						onConnectClient(Instance, Overlapped, IoResult, nBytesTransfered, pIo);
					});

					if( pPool->StartThreadpoolIo(m_pio.get()) )
					{
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
							pPool->CancelThreadpoolIo(m_pio.get());	// prevent a memory leak if ConnectNamedPipe() fails.
						}
					}
					else
						_ftprintf_s(stderr, TEXT("StartThreadpoolIo failed. GLE=%d\n"), GetLastError());
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
ListenForDevRandomClient::onConnectClient(PTP_CALLBACK_INSTANCE , PVOID Overlapped, ULONG IoResult, ULONG_PTR, IIoCompletion *pIo)
{
	TRACE(TEXT(">>>ListenForDevRandomClient::onConnectClient() enter\n")); 
	IThreadPool *pPool = NULL;
	bool bReSubmit = false;

	if( !m_stop )
	{
		if( pIo )
		{
			pPool = pIo->pool();
			if( pPool )
			{
				IIoCompletionPtr pio = ::std::static_pointer_cast<IIoCompletion>(pIo->shared_from_this());
				MyOverlappedPtr olp(m_olp);
				MyOverlapped *pOlp = reinterpret_cast<MyOverlapped *>(Overlapped);
				bReSubmit = true;
				if( pOlp )
				{
					if( NO_ERROR == IoResult)
					{
						DevRandomClientConnection::Ptr pClient = DevRandomClientConnection::create(pio, m_hPipe, olp, m_hStopEvent, pPool);

						if( pClient )
						{
							pClient->makeSelfReference();
							if( !pClient->runClient() )
								pClient.reset();
						}
					}
					else
						_ftprintf_s(stderr, TEXT("ListenForDevRandomClient::onConnectClient IoResult == %d\n"), IoResult);

				}
				else
					_ftprintf_s(stderr, TEXT("ListenForDevRandomClient::onConnectClient pOlp == NULL \n"));
			}
			else
				_ftprintf_s(stderr, TEXT("ListenForDevRandomClient::onConnectClient pPool == NULL \n"));
		}
		else
			_ftprintf_s(stderr, TEXT("ListenForDevRandomClient::onConnectClient pIo == NULL \n"));

		TRACE(TEXT(">>>ListenForDevRandomClient::onConnectClient() exit\n")); 
		if( pPool && bReSubmit )
		{
			if( !pPool->SubmitThreadpoolWork(m_work.get()) )
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
