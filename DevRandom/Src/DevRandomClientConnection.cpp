#include "stdafx.h"
#include "DevRandomClientConnection.h"
#include "SpinLock.h"

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
	DECLARE_TIME_ACQUIRE_LOCK();
public:
	ExclusiveLock(PSRWLOCK pLock) : m_pLock(pLock)
	{
		BEGIN_TIME_ACQUIRE_LOCK();
		AcquireSRWLockExclusive(m_pLock);
		END_TIME_ACQUIRE_LOCK();
	}
	~ExclusiveLock()
	{
		ReleaseSRWLockExclusive(m_pLock);
	}
};


#ifdef TRACK_SPIN_COUNTS
static
class Max_ExclusiveLock_Report
{
public:
	Max_ExclusiveLock_Report() {}
	~Max_ExclusiveLock_Report()
	{
		//TRACE(TEXT("LWSpinLock max iterations executed was: %d\n"), LWSpinLock::MaxIter());
		TCHAR buffer[2048];
		_stprintf_s(buffer,TEXT("ExclusiveLock max lock cycles count executed was:\t%I64u\nTotal cycles spent waiting was:\t\t\t%I64u\nAverage Cycles spent waiting was:\t\t\t%I64u\n"), ExclusiveLock::MaxDt(), ExclusiveLock::Sum(), ExclusiveLock::Sum() / ExclusiveLock::Calls() );
		::OutputDebugString(buffer);
//		_stprintf_s(buffer,TEXT("LWTrySpinLocker max lock cycles count executed was:\t%I64u\nTotal cycles spent spinning was:\t\t%I64u\nAverage Cycles spent spinning was:%I64u\n"), LWTrySpinLocker::MaxDt(), LWTrySpinLocker::Sum(), LWTrySpinLocker::Sum() / LWTrySpinLocker::Calls() );
//		::OutputDebugString(buffer);
	}
}_____fooReportIt;
#endif


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

DevRandomClientConnection::DevRandomClientConnection(HANDLE hPipe, const MyOverlappedPtr &olp, HANDLE hStopEvent, IThreadPool *pPool) :
m_hPipe(hPipe),
m_olp(olp),
m_stop(false)
{
	InitializeSRWLock(&m_SRWLock);
	interlockedIncrement(&m_nActiveClients);
	TRACE(TEXT(">>>DevRandomClientConnection::DevRandomClientConnection(), this=0x%x.\n"), this); 
	m_work = pPool->newWork([&](PTP_CALLBACK_INSTANCE Instance, IWork *pWork) {
		writeToClient(Instance, pWork);
	});

	if( !m_work )
		throw ::std::runtime_error("DevRandomClientConnection::DevRandomClientConnection() - newWork() failed");

	m_pio = pPool->newIoCompletion(hPipe, [&](PTP_CALLBACK_INSTANCE Instance, PVOID Overlapped, ULONG IoResult, ULONG_PTR nBytesTransfered, IIoCompletion *pIo){
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
	IThreadPool *pPool = m_pio ? m_pio->pool() : (m_work ? m_work->pool() : NULL) ;
	if( pPool && pPool->Enabled() )
	{
		if( m_work)
			m_work->CloseWork();

		if( m_pio )
			m_pio->CloseIo();

		if( m_wait )
			m_wait->CloseWait();

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
DevRandomClientConnection::makeSelfReferent()
{
	m_self = this->shared_from_this();	// self-reference - we live in the thread pool 
}

//static 
DevRandomClientConnection::Ptr
DevRandomClientConnection::create(HANDLE hPipe, const MyOverlappedPtr &olp, HANDLE hStopEvent, IThreadPool *pPool)
{
#ifdef _DEBUG
	return ::std::shared_ptr<DevRandomClientConnection>(new DevRandomClientConnection(hPipe, olp, hStopEvent, pPool));
#else
	return ::std::make_shared<DevRandomClientConnection>(hPipe, olp, hStopEvent, pPool);
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
	if( m_hPipe != INVALID_HANDLE_VALUE )
	{
		DisconnectNamedPipe(m_hPipe);	// dis-connect the client
		CloseHandle(m_hPipe);			// close the handle
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
			TRACE(TEXT("WriteData(): bad pipe handle\n"));
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
	if( !m_stop )
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

void
DevRandomClientConnection::onWriteClientComplete(PTP_CALLBACK_INSTANCE , PVOID /*Overlapped*/, ULONG IoResult, ULONG_PTR, IIoCompletion * /*pIo*/)
{
	if( !m_stop )
	{
		SharedLock lock(&m_SRWLock);
		if( lock.locked() )
		{
			if( NO_ERROR == IoResult )
			{
				if( !m_work->SubmitWork() )
				{
					Stop(IO_CALL);
				}
			}
			else
				Stop(IO_CALL);
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
DevRandomClientConnection::doStop(PTP_CALLBACK_INSTANCE Instance, IWork * pWork)
{
	{
		ExclusiveLock lock(&m_SRWLock);
		closeHandle();
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
	if( interlockedExchange(&m_stop,1) == 0 )
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
