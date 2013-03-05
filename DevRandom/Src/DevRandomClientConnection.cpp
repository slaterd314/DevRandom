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

DevRandomClientConnection::DevRandomClientConnection(const IIoCompletionPtr &pio, HANDLE hPipe, const MyOverlappedPtr &olp, HANDLE hStopEvent, IThreadPool *pPool) :
m_pio(pio),
m_hPipe(hPipe),
m_olp(olp),
m_lock(false),
m_asyncIo(false),
m_asyncWork(false),
m_stop(false)
{
	InitializeSRWLock(&m_SRWLock);
	interlockedIncrement(&m_nActiveClients);
	TRACE(TEXT(">>>DevRandomClientConnection::DevRandomClientConnection(), this=0x%x.\n"), this); 
	m_work = pPool->newWork([&](PTP_CALLBACK_INSTANCE Instance, IWork *pWork) {
		writeToClient(Instance, pWork);
	});

	m_pio->setIoComplete([&](PTP_CALLBACK_INSTANCE Instance, PVOID Overlapped, ULONG IoResult, ULONG_PTR nBytesTransfered, IIoCompletion *pIo){
		onWriteClientComplete(Instance, Overlapped, IoResult, nBytesTransfered, pIo);
	});

	if( hStopEvent )
	{
		m_wait = pPool->newWait([&](PTP_CALLBACK_INSTANCE Instance, TP_WAIT_RESULT WaitResult, IWait *pWait) {
			onWaitSignaled(Instance,WaitResult,pWait);
		});

		if( m_wait )
		{
			pPool->SetThreadpoolWait(hStopEvent,NULL,m_wait.get());
		}
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
			pPool->CloseThreadpoolWork(m_work.get());
		}
		if( m_pio )
		{
			//::WaitForThreadpoolIoCallbacks(m_pio->handle(), FALSE);
			pPool->CloseThreadpoolIo(m_pio.get());
		}
		if( m_wait )
		{
			//::WaitForThreadpoolWaitCallbacks(m_wait->handle(),TRUE);
			pPool->CloseThreadpoolWait(m_wait.get());
		}				
	}

	m_work->setWork(NULL);
	m_pio->setIoComplete(NULL);

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
DevRandomClientConnection::create(const IIoCompletionPtr &pio, HANDLE hPipe, const MyOverlappedPtr &olp, HANDLE hStopEvent, IThreadPool *pPool)
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
DevRandomClientConnection::writeToClient(PTP_CALLBACK_INSTANCE /*Instance*/, IWork * /*pWork*/)
{
	//lock locked(m_lock);
	if( !m_stop )
	{
		m_asyncWork = false;
		SharedLock lock(&m_SRWLock);
		if( lock.locked() )
		{
			bool bKeepWriting = true;
			DWORD cbReplyBytes = MyOverlapped::BUFSIZE;
			DWORD cbWritten = 0;

			unsigned __int8 *pWritePtr = m_olp->buffer;
			m_olp->swapBuffers();
			unsigned __int8 *pGeneratePtr = m_olp->buffer;

			if( m_pio.get() )
			{
				m_asyncIo = true;
				m_pio->pool()->StartThreadpoolIo(m_pio.get());
				// Write the reply to the pipe. 
				BOOL fSuccess = WriteFile( 
							m_hPipe,        // handle to pipe 
							pWritePtr,     // buffer to write from 
							cbReplyBytes, // number of bytes to write 
							&cbWritten,   // number of bytes written 
							m_olp.get());        // overlapped I/O 

				if(!fSuccess)
					fSuccess = checkWriteFileError();

				if (!fSuccess /*|| cbReplyBytes != cbWritten*/ )
				{   
					m_asyncIo = false;
					_ftprintf_s(stderr,TEXT("WriteData::operator() failed, GLE=%d.\n"), GetLastError());
					bKeepWriting = false;
				}
				else if( FALSE == RtlGenRandom(pGeneratePtr, MyOverlapped::BUFSIZE) )
				{
					m_pio->pool()->CancelThreadpoolIo(m_pio.get());
					bKeepWriting = false;
				}
			}
			else
				bKeepWriting = false;
			// release our hold on the work object that contains us.
			// this should trigger deletion of the work object and everything else associated with it.
			if( !bKeepWriting  )
			{
				//if( m_self )
				//	m_self.reset();	// release ourselves from the thread pool			
				Stop(WORK_CALL);
			}
		}
	}
}

void
DevRandomClientConnection::onWriteClientComplete(PTP_CALLBACK_INSTANCE , PVOID /*Overlapped*/, ULONG IoResult, ULONG_PTR, IIoCompletion *pIo)
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
				if( !pIo->pool()->SubmitThreadpoolWork(m_work.get()) )
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
		bRetVal = m_work->pool()->SubmitThreadpoolWork(m_work.get());
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
DevRandomClientConnection::doStop(PTP_CALLBACK_INSTANCE /*Instance*/, IWork * pWork)
{	
	::std::unique_ptr<TryExclusiveLock> lock(new TryExclusiveLock(&m_SRWLock));
	if( lock && lock->locked() )
	{
		IThreadPool *pPool = pWork ? pWork->pool() : NULL;
		if( pPool )
		{
			if( INVALID_HANDLE_VALUE != m_hPipe )
			{
				//HANDLE h = InterlockedExchangePointer(&m_hPipe, INVALID_HANDLE_VALUE);
				CloseHandle(m_hPipe);
				m_hPipe = INVALID_HANDLE_VALUE;
				if( m_pio && m_asyncIo)
				{
					pPool->WaitForThreadpoolIoCallbacks(m_pio.get(), TRUE);
					//pPool->CloseThreadpoolIo(m_pio.get());
				}
			}
			if( m_work && m_asyncWork)
				pPool->WaitForThreadpoolWorkCallbacks(m_work.get(), TRUE);
			pPool->CloseThreadpoolWork(pWork);
		}
		lock.reset();
		m_self.reset();
	}
	else if( pWork )
	{
		// re-submit the work packet
		bool bRetVal = pWork->pool()->SubmitThreadpoolWork(pWork);
		UNREFERENCED_PARAMETER(bRetVal);
	}

}

void
DevRandomClientConnection::Stop(StopCaller /*caller*/)
{
	if( interlockedCompareExchange(&m_stop,1,0) == 0 )
	{
		IThreadPool *pPool = (m_work ? m_work->pool() : (m_pio ? m_pio->pool() : NULL));
		if( pPool )
		{
			m_workStop = pPool->newWork([&](PTP_CALLBACK_INSTANCE Instance, IWork * pWork) {
				doStop(Instance, pWork);
			});
			if( m_workStop )
			{
				pPool->SubmitThreadpoolWork(m_workStop.get());
			}
		}
	}
}
