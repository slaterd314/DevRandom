#include "stdafx.h"
#include "DevRandomClientConnection.h"

volatile ALIGN MACHINE_INT DevRandomClientConnection::m_nActiveClients = 0;

class SharedLock
{
	PSRWLOCK m_pLock;
	bool m_bLocked;
public:
	SharedLock(PSRWLOCK pLock) : m_pLock(pLock), m_bLocked(false)
	{
		m_bLocked = (0!=TryAcquireSRWLockShared(m_pLock));
	}
	const bool &locked()const { return m_bLocked; }
	~SharedLock()
	{
		if( m_bLocked )
			ReleaseSRWLockShared(m_pLock);
	}
};

class ExclusiveLock
{
	PSRWLOCK m_pLock;
public:
	ExclusiveLock(PSRWLOCK pLock) : m_pLock(pLock)
	{
		AcquireSRWLockExclusive(m_pLock);
	}
	~ExclusiveLock()
	{
		ReleaseSRWLockExclusive(m_pLock);
	}
};

class TryExclusiveLock
{
	PSRWLOCK m_pLock;
	bool m_bLocked;
public:
	TryExclusiveLock(PSRWLOCK pLock) : m_pLock(pLock), m_bLocked(false)
	{
		m_bLocked = (0!=TryAcquireSRWLockExclusive(m_pLock));
	}
	const bool &locked()const { return m_bLocked; }
	~TryExclusiveLock()
	{
		if( m_bLocked )
			ReleaseSRWLockExclusive(m_pLock);
	}
};

DevRandomClientConnection::DevRandomClientConnection(const IIoCompletion::Ptr &pio, HANDLE hPipe, const MyOverlappedPtr &olp, HANDLE hStopEvent, IThreadPool *pPool) :
m_pio(pio),
m_hPipe(hPipe),
m_olp(olp),
m_asyncIo(false),
m_asyncWork(false),
m_stop(false),
m_nOutStandingIoOps(0)
{
	InitializeSRWLock(&m_SRWLock);
	interlockedIncrement(&m_nActiveClients);
	TRACE(TEXT(">>>DevRandomClientConnection::DevRandomClientConnection(), this=0x%x.\n"), this); 
	m_work = pPool->newWork([&](PTP_CALLBACK_INSTANCE Instance, IWork *pWork) {
		writeToClient(Instance, pWork);
	});

	if( !m_work )
		throw ::std::runtime_error("DevRandomClientConnection::DevRandomClientConnection() - newWork() failed");

	m_pio->setIoComplete([&](PTP_CALLBACK_INSTANCE Instance, PVOID Overlapped, ULONG IoResult, ULONG_PTR nBytesTransfered, IIoCompletion *pIo){
		onWriteClientComplete(Instance, Overlapped, IoResult, nBytesTransfered, pIo);
	});

	if( hStopEvent )
	{
		m_wait = pPool->newWait([&](PTP_CALLBACK_INSTANCE Instance, TP_WAIT_RESULT WaitResult, IWait *pWait) {
			onWaitSignaled(Instance,WaitResult,pWait);
		});

		if( !m_wait )
			throw ::std::runtime_error("DevRandomClientConnection::DevRandomClientConnection() - newWait() failed");

		m_wait->SetThreadpoolWait(hStopEvent,NULL);

	}
	m_olp->buffer = m_olp->buffer1;
	RtlGenRandom(m_olp->buffer, MyOverlapped::BUFSIZE);
}

DevRandomClientConnection::~DevRandomClientConnection()
{
	//lock locked(m_lock);
	//ExclusiveLock lock(&m_SRWLock);
	IThreadPool *pPool = m_pio ? m_pio->pool() : (m_work ? m_work->pool() : NULL) ;
	if( pPool && pPool->Enabled() )
	{
		if( m_work)
		{
			//::WaitForThreadpoolWorkCallbacks(m_work->handle(), TRUE);
			m_work->CloseWork();
		}
		if( m_pio )
		{
			//::WaitForThreadpoolIoCallbacks(m_pio->handle(), FALSE);
			m_pio->CloseIo();
		}
		if( m_wait )
		{
			//::WaitForThreadpoolWaitCallbacks(m_wait->handle(),TRUE);
			m_wait->CloseWait();
		}				
	}

	if( m_work )
	{
		m_work->setWork(NULL);
		m_work.reset();
	}
	if( m_workStop )
	{
		m_workStop->setWork(NULL);
		m_workStop.reset();
	}
	if( m_pio )
	{
		m_pio->setIoComplete(NULL);
		m_pio.reset();
	}
	if( m_wait )
	{
		m_wait->setWait(NULL);
		m_wait.reset();
	}
	if( m_olp )
	{
		if( m_olp->hEvent )
		{
			CloseHandle(m_olp->hEvent);
			m_olp->hEvent = NULL;
		}
		m_olp.reset();
	}

	if( INVALID_HANDLE_VALUE != m_hPipe )
		CloseHandle(m_hPipe);
	m_hPipe = INVALID_HANDLE_VALUE;
	TRACE(TEXT("<<<DevRandomClientConnection::~DevRandomClientConnection(), this=0x%x.\n"), this); 
	interlockedDecrement(&m_nActiveClients);
}

// static
void
DevRandomClientConnection::waitForClientsToStop()
{
	while(m_nActiveClients>0)
	{
		Sleep(1);
	}
}

void
DevRandomClientConnection::makeSelfReference()
{
	m_self = this->shared_from_this();	// self-reference - we live in the thread pool 
}

//static 
DevRandomClientConnection::Ptr
DevRandomClientConnection::create(const IIoCompletion::Ptr &pio, HANDLE hPipe, const MyOverlappedPtr &olp, HANDLE hStopEvent, IThreadPool *pPool)
{
#ifdef _DEBUG
	return ::std::shared_ptr<DevRandomClientConnection>(new DevRandomClientConnection(pio, hPipe, olp, hStopEvent, pPool));
#else
	return ::std::make_shared<DevRandomClientConnection>(pio, hPipe, olp, hStopEvent, pPool);
#endif
}

bool
DevRandomClientConnection::checkWriteFileError()
{
	bool bWriteSuccess = false;
	switch(GetLastError())
	{
		case ERROR_IO_PENDING:
			bWriteSuccess = true;
			break;
		default:
			break;
	}
	return bWriteSuccess;
}

void
DevRandomClientConnection::closeHandle()
{
	ExclusiveLock lock(&m_SRWLock);
	if( m_hPipe != INVALID_HANDLE_VALUE )
	{
		CloseHandle(m_hPipe);
		m_hPipe = INVALID_HANDLE_VALUE;
	}
}

bool
DevRandomClientConnection::WriteData(const unsigned __int8 *pData, const DWORD &dwDataLength)
{
	bool bRetVal = false;
	SharedLock lock(&m_SRWLock);
	if( lock.locked() )
	{
		if( m_hPipe != INVALID_HANDLE_VALUE )
		{
			if( m_pio.get() )
			{
				bRetVal = m_pio->StartIo();
				if( bRetVal )
				{
					m_asyncIo = true;
					DWORD cbWritten = 0;
					BOOL fSuccess = WriteFile( 
								m_hPipe,        // handle to pipe 
								pData,     // buffer to write from 
								dwDataLength, // number of bytes to write 
								&cbWritten,   // number of bytes written 
								m_olp.get());        // overlapped I/O 
					if(!fSuccess)
						fSuccess = checkWriteFileError();
					if( !fSuccess )
					{
						bRetVal = false;
						m_asyncIo = false;
						_ftprintf_s(stderr,TEXT("WriteFile() failed, GLE=%d.\n"), GetLastError());
						m_pio->CancelIo();
					}
				}
				else
				{
					TRACE(TEXT("WriteData(): m_pio->StartIo() failed\n"));
				}
			}
			else
			{
				TRACE(TEXT("WriteData(): m_piois NULL \n"));
			}
		}
		else
		{
			TRACE(TEXT("WriteData(): m_lock is locked\n"));
		}
	}
	else
	{
		TRACE(TEXT("WriteData(): SharedLock::tryLock failed\n"));
	}
	return bRetVal;
}

void
DevRandomClientConnection::writeToClient(PTP_CALLBACK_INSTANCE /*Instance*/, IWork * /*pWork*/)
{
	//lock locked(m_lock);
	if( !m_stop )
	{
		m_asyncWork = false;
		//SharedLock lock(&m_SRWLock);
		//if( lock.locked() )
		{
			bool bKeepWriting = true;

			unsigned __int8 *pWritePtr = m_olp->buffer;
			m_olp->swapBuffers();
			unsigned __int8 *pGeneratePtr = m_olp->buffer;

			if( !WriteData(pWritePtr, MyOverlapped::BUFSIZE) )
			{
				bKeepWriting = false;
			}
			else if( FALSE == RtlGenRandom(pGeneratePtr, MyOverlapped::BUFSIZE) )
			{
				bKeepWriting = false;
			}

			if( !bKeepWriting  )
			{
				Stop(WORK_CALL);
			}
		}
	}
}

void
DevRandomClientConnection::onWriteClientComplete(PTP_CALLBACK_INSTANCE , PVOID /*Overlapped*/, ULONG IoResult, ULONG_PTR, IIoCompletion * /*pIo*/)
{
	if( !m_stop )
	{
		m_asyncIo = false;
		SharedLock lock(&m_SRWLock);
		if( lock.locked() )
		{
			if( NO_ERROR == IoResult )
			{
				//lock locked(m_lock);
				m_asyncWork = true;
				if( !m_work->SubmitWork() )
				{
					Stop(IO_CALL);// m_self.reset();
					m_asyncWork = false;
				}
			}
			else
				Stop(IO_CALL);// m_self.reset();
		}
	}
}

bool
DevRandomClientConnection::runClient()
{
	bool bRetVal = false;
	if( m_work && m_pio && m_hPipe != INVALID_HANDLE_VALUE )
	{
		bRetVal = m_work->SubmitWork();
	}
	return bRetVal;
}

void
DevRandomClientConnection::onWaitSignaled(PTP_CALLBACK_INSTANCE , TP_WAIT_RESULT /*WaitResult*/, IWait * /*pWait*/)
{
//	ExclusiveLock lock(&m_SRWLock);
	Stop(WAIT_CALL);// 
}

void
DevRandomClientConnection::doStop2(PTP_CALLBACK_INSTANCE Instance, IThreadPool * pPool)
{
	closeHandle();
	{
		ExclusiveLock lock(&m_SRWLock);
		if( pPool )
		{
			CallbackMayRunLong(Instance);
			m_work->WaitForCallbacks(TRUE);
			m_pio->WaitForCallbacks(TRUE);
		}
	}
	m_self.reset();
}


void
DevRandomClientConnection::doStop(PTP_CALLBACK_INSTANCE Instance, IWork * pWork)
{
	closeHandle();
	{
		ExclusiveLock lock(&m_SRWLock);
		if( pWork && pWork->pool() )
		{
			CallbackMayRunLong(Instance);
			m_work->WaitForCallbacks(TRUE);
			m_pio->WaitForCallbacks(TRUE);
		}
	}
	m_self.reset();
}


void
DevRandomClientConnection::Stop(StopCaller /*caller*/)
{
	if( interlockedCompareExchange(&m_stop,1,0) == 0 )
	{
		IThreadPool *pPool = (m_work ? m_work->pool() : (m_pio ? m_pio->pool() : NULL));
		if( pPool )
		{
			IThreadPool *pPool = IThreadPool::getDefaultPool();
			if( pPool )
			{
				m_workStop = pPool->newWork([&](PTP_CALLBACK_INSTANCE Instance, IWork * pWork) {
					doStop(Instance, pWork);
				});
				if( m_workStop )
				{
					m_workStop->SubmitWork();
				}
			}
		}
	}
}
