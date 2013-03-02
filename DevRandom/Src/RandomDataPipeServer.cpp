#include "stdafx.h"
#include "IIoCompletion.h"
#include "IThreadPool.h"
#include "IWork.h"
#include "SpinLock.h"
#include "RandomDataPipeServer.h"

#ifdef _DEBUG
#define new DEBUG_NEW

#endif


struct MyOverlapped : public OVERLAPPED, public ::std::enable_shared_from_this<MyOverlapped>
{
	enum { BUFSIZE=256 };
	unsigned __int8 *buffer;
	unsigned __int8 buffer1[BUFSIZE];
	unsigned __int8 buffer2[BUFSIZE];
	MyOverlapped()
	{
		reset();
	}
	MyOverlapped(const MyOverlapped &other) : OVERLAPPED(static_cast<const OVERLAPPED &>(other))
	{
		memcpy(buffer1,other.buffer1,sizeof(buffer1));
		memcpy(buffer2,other.buffer2,sizeof(buffer2));
		if( other.buffer == other.buffer1 )
			buffer = buffer1;
		else
			buffer = buffer2;

	}
	void reset()
	{
		memset(buffer1,'\0',sizeof(buffer1));
		memset(buffer2,'\0',sizeof(buffer2));
		OVERLAPPED *pOlp = static_cast<OVERLAPPED *>(this);
		memset(pOlp, '\0', sizeof(OVERLAPPED));
		buffer = buffer1;
	}
	void swapBuffers()
	{
		if( buffer == buffer1 )
			buffer = buffer2;
		else
			buffer = buffer1;
	}
};

typedef ::std::shared_ptr<MyOverlapped> MyOverlappedPtr;

//class WorkItem : public IWork::FuncPtr
//{
//	::std::shared_ptr<WorkItem> m_ptr;
//protected:
//	IWorkPtr		m_work;
//	void setWork(const IWorkPtr &pWork)
//	{
//		m_work = pWork;
//	}
//protected:
//	void release()
//	{
//		if( m_ptr )
//			m_ptr.reset();
//	}
//public:
//	const IWorkPtr &getWork()const 
//	{
//		return m_work;
//	}
//	void setPointer(const ::std::shared_ptr<WorkItem> &p)
//	{
//		m_ptr = p;
//	}
//};

class DevRandomClientConnection : public ::std::enable_shared_from_this<DevRandomClientConnection>
{
	struct lock
	{
		SpinLock &m_lock;
		lock(SpinLock &l) : m_lock(l) { m_lock.acquire(); }
		~lock() { m_lock.release(); }
	private:
		lock &operator=(const lock &) { return *this; }
	};
public:
	typedef ::std::shared_ptr<class DevRandomClientConnection> Ptr;

	DevRandomClientConnection(const IIoCompletionPtr &pio, HANDLE hPipe, const MyOverlapped &olp, HANDLE hStopEvent, IThreadPool *pPool) :
		m_pio(pio),
		m_hPipe(hPipe),
		m_olp(olp),
		m_lock(false)
	{
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
		m_olp.buffer = m_olp.buffer1;
		RtlGenRandom(m_olp.buffer, MyOverlapped::BUFSIZE);

	}

	void makeSelfReference()
	{
		m_self = this->shared_from_this();	// self-reference - we live in the thread pool 
	}
public:
	static Ptr create(const IIoCompletionPtr &pio, HANDLE hPipe, const MyOverlapped &olp, HANDLE hStopEvent, IThreadPool *pPool)
	{
		return ::std::make_shared<DevRandomClientConnection>(pio, hPipe, olp, hStopEvent, pPool);
	}

	bool checkWriteFileError()
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

	void writeToClient(PTP_CALLBACK_INSTANCE /*Instance*/, IWork * /*pWork*/)
	{
		//lock locked(m_lock);
		bool bKeepWriting = true;
		DWORD cbReplyBytes = MyOverlapped::BUFSIZE;
		DWORD cbWritten = 0;

		unsigned __int8 *pWritePtr = m_olp.buffer;
		m_olp.swapBuffers();
		unsigned __int8 *pGeneratePtr = m_olp.buffer;

		if( m_pio.get() )
		{
			m_pio->pool()->StartThreadpoolIo(m_pio.get());
			// Write the reply to the pipe. 
			BOOL fSuccess = WriteFile( 
						m_hPipe,        // handle to pipe 
						pWritePtr,     // buffer to write from 
						cbReplyBytes, // number of bytes to write 
						&cbWritten,   // number of bytes written 
						&m_olp);        // overlapped I/O 

			if(!fSuccess)
				fSuccess = checkWriteFileError();

			if (!fSuccess /*|| cbReplyBytes != cbWritten*/ )
			{   
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
			if( m_self )
				m_self.reset();	// release ourselves from the thread pool
		}
	}

	void onWriteClientComplete(PTP_CALLBACK_INSTANCE , PVOID /*Overlapped*/, ULONG IoResult, ULONG_PTR, IIoCompletion *pIo)
	{
		if( NO_ERROR == IoResult )
		{
			//lock locked(m_lock);
			if( !pIo->pool()->SubmitThreadpoolWork(m_work.get()) )
				m_self.reset();
		}
		else
			m_self.reset();
	}

	bool runClient()
	{
		bool bRetVal = false;
		if( m_work && m_pio && m_hPipe != INVALID_HANDLE_VALUE )
		{
			bRetVal = m_work->pool()->SubmitThreadpoolWork(m_work.get());
		}
		return bRetVal;
	}

	void onWaitSignaled(PTP_CALLBACK_INSTANCE , TP_WAIT_RESULT /*WaitResult*/, IWait * /*pWait*/)
	{
		if( m_wait )
			m_wait.reset();
		m_self.reset();
	}

	~DevRandomClientConnection()
	{
		//lock locked(m_lock);
		IThreadPool *pPool = m_pio ? m_pio->pool() : (m_work ? m_work->pool() : NULL) ;
		if( pPool && pPool->Enabled() )
		{
			if( m_work)
			{
				::WaitForThreadpoolWorkCallbacks(m_work->handle(), TRUE);
				pPool->CloseThreadpoolWork(m_work.get());
			}
			if( m_pio )
			{
				::WaitForThreadpoolIoCallbacks(m_pio->handle(), FALSE);
				pPool->CloseThreadpoolIo(m_pio.get());
			}
			if( m_wait )
			{
				::WaitForThreadpoolWaitCallbacks(m_wait->handle(),TRUE);
				pPool->CloseThreadpoolWait(m_wait.get());
			}				
		}

		if( INVALID_HANDLE_VALUE != m_hPipe )
			CloseHandle(m_hPipe);
	}

private:
	IWorkPtr			m_work;
	IWaitPtr			m_wait;
	IIoCompletionPtr	m_pio;
	HANDLE				m_hPipe;
	MyOverlapped		m_olp;
	Ptr					m_self;
	SpinLock			m_lock;
};

class ListenForDevRandomClient : public ::std::enable_shared_from_this<ListenForDevRandomClient>, public IDevRandomServer
{
public:
	typedef ::std::shared_ptr<class ListenForDevRandomClient> Ptr;
private:
	IWorkPtr			m_work;
	IIoCompletionPtr	m_pio;
	HANDLE				m_hPipe;
	MyOverlapped		m_olp;
	::std::_tstring		m_pipeName;
	Ptr					m_self;
	HANDLE				m_hStopEvent;
public:
	typedef ::std::shared_ptr<class ListenForDevRandomClient> Ptr;
	ListenForDevRandomClient(LPCTSTR lpszPipeName, HANDLE hStopEvent, IThreadPool *pPool) : m_hPipe(INVALID_HANDLE_VALUE), m_pipeName(lpszPipeName), m_hStopEvent(hStopEvent)
	{
		m_work = pPool->newWork([&](PTP_CALLBACK_INSTANCE Instance, IWork *pWork) {
			listenForClient(Instance, pWork);
		});
	}

	virtual bool runServer()
	{
		bool bRetVal = false;
		if( m_work && m_work->pool() )
		 bRetVal = m_work->pool()->SubmitThreadpoolWork(m_work.get());
		return bRetVal;
	}

	void makeSelfReference()
	{
		// self-reference
		m_self = shared_from_this();
	}

	static Ptr create(LPCTSTR lpszPipeName, HANDLE hStopEvent, IThreadPool *pPool)
	{
		return ::std::make_shared<ListenForDevRandomClient>(lpszPipeName, hStopEvent, pPool);
	}

	bool startServer()
	{
		bool bRetVal = false;
		if( m_work && m_work->pool() )
			bRetVal = m_work->pool()->SubmitThreadpoolWork(m_work.get());
		return bRetVal;
	}

	void listenForClient(PTP_CALLBACK_INSTANCE , IWork *pWork)
	{
		if( pWork && pWork->pool() )
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
				m_olp.reset();

				if( RtlGenRandom(m_olp.buffer, MyOverlapped::BUFSIZE) )
				{				
			
					m_pio = pPool->newIoCompletion(m_hPipe, [&](PTP_CALLBACK_INSTANCE Instance, PVOID Overlapped, ULONG IoResult, ULONG_PTR nBytesTransfered, IIoCompletion *pIo){
						onConnectClient(Instance, Overlapped, IoResult, nBytesTransfered, pIo);
					});

					pPool->StartThreadpoolIo(m_pio.get());
			
					BOOL fConnected = ConnectNamedPipe(m_hPipe, &m_olp);	// asynchronous ConnectNamedPipe() call - this call will return immediately
																		// and the Io completion routine will be called when a client connects.
					if( !fConnected )
					{
						if( GetLastError() == ERROR_IO_PENDING || GetLastError() == ERROR_PIPE_CONNECTED )
							fConnected = TRUE;
					}
					if( !fConnected )
					{
						pPool->CancelThreadpoolIo(m_pio.get());	// prevent a memory leak if ConnectNamedPipe() fails.
					}
				}
			}
		}
	}


	void onConnectClient(PTP_CALLBACK_INSTANCE , PVOID Overlapped, ULONG IoResult, ULONG_PTR, IIoCompletion *pIo)
	{
		MyOverlapped *pOlp = reinterpret_cast<MyOverlapped *>(Overlapped);
		if( pIo && (NO_ERROR == IoResult) && pOlp)
		{		
			IThreadPool *pPool = pIo->pool();
			
			DevRandomClientConnection::Ptr pClient = ::std::make_shared<DevRandomClientConnection>(m_pio,m_hPipe, m_olp, m_hStopEvent, pPool);

			if( pClient )
			{
				pClient->makeSelfReference();
				pClient->runClient();
			}

			m_pio.reset();
			m_hPipe = INVALID_HANDLE_VALUE;

			// start listening again
			if( !pPool->SubmitThreadpoolWork(m_work.get()) )
				m_self.reset();
		}
	}
};

IDevRandomServer::Ptr createPipeServer(LPCTSTR lpszPipeName, HANDLE hStopEvent, IThreadPool *pPool)
{
	IDevRandomServer::Ptr pServer;
	if( pPool )
	{
		pServer = ListenForDevRandomClient::create(lpszPipeName, hStopEvent, pPool);

		//if( pServer )
		//{
		//	pServer->makeSelfReference();
		//	//bRetVal = pServer->startServer();
		//}
	}
	return pServer;
}


#ifdef NEVER

class Connect//  : public IWork::FuncPtr
{
	IIoCompletionPtr m_pio;
	::std::shared_ptr<MyOverlapped> m_olp;
	LPCTSTR lpszPipeName;
	::std::shared_ptr<Connect> m_ptr;
	IWorkPtr		m_work;
public:
	Connect(const Connect &other) : 
		m_pio(other.m_pio), 
		m_olp(other.m_olp),
		lpszPipeName(other.lpszPipeName),
		m_ptr(other.m_ptr),
		m_work(other.m_work)
	{
	}
	Connect(LPCTSTR lpsz, IThreadPool *pPool);
	void operator()(PTP_CALLBACK_INSTANCE , IWork *pWork);
	void setPointer(const ::std::shared_ptr<Connect> &p)
	{
		m_ptr = p;
	}
	void release()
	{
		if( m_ptr )
			m_ptr.reset();
	}
	void setWork(const IWorkPtr &pWork)
	{
		m_work = pWork;
	}
	const IWorkPtr &getWork()const 
	{
		return m_work;
	}
};

class OnConnect // : public IIoCompletion::FuncPtr
{
	::std::weak_ptr<IWork> m_work;
	HANDLE m_hPipe;
public:
	OnConnect(HANDLE h, const IWorkPtr &pWork);
	void operator()(PTP_CALLBACK_INSTANCE , PVOID Overlapped, ULONG IoResult, ULONG_PTR, IIoCompletion *pIo);
	~OnConnect()
	{
	}
};

class WriteData : public ::std::enable_shared_from_this<WriteData> //  : public IWork::FuncPtr
{
	IIoCompletionPtr m_pio;
	HANDLE hPipe;
	MyOverlappedPtr m_olp;
	::std::shared_ptr<WriteData> m_ptr;
	IWorkPtr		m_work;
public:
	WriteData(HANDLE h, IIoCompletion *pio, const MyOverlappedPtr &olp, IThreadPool *pPool);
	void operator()(PTP_CALLBACK_INSTANCE , IWork *pWork);
	void setPointer(const ::std::shared_ptr<WriteData> &p)
	{
		m_ptr = p;
	}
	void release()
	{
		if( m_ptr )
			m_ptr.reset();
	}
	void setWork(const IWorkPtr &pWork)
	{
		m_work = pWork;
	}
	const IWorkPtr &getWork()const 
	{
		return m_work;
	}
};

class KickWrite //: public IIoCompletion::FuncPtr
{
	::std::weak_ptr<IWork> m_work;
public:
	KickWrite(const IWorkPtr &pWork);
	void operator()(PTP_CALLBACK_INSTANCE , PVOID, ULONG IoResult, ULONG_PTR, IIoCompletion *pIo);

};

Connect::Connect(LPCTSTR lpsz, IThreadPool * /*pPool*/) : lpszPipeName(lpsz)
{
	//if( pPool )
	//{
	//	setWork(pPool->newWork(IWork::FuncPtr(*this)));
	//}
}

void
Connect::operator()(PTP_CALLBACK_INSTANCE , IWork *pWork)
{
	if( pWork && pWork->pool() )
	{
		IThreadPool *pPool = pWork->pool();
		// IWorkPtr pItem(::std::dynamic_pointer_cast<IWork>(pWork->shared_from_this()));
			
		m_olp.reset(new MyOverlapped);

		HANDLE hPipe = CreateNamedPipe(lpszPipeName,
						PIPE_ACCESS_OUTBOUND|FILE_FLAG_OVERLAPPED,
						PIPE_TYPE_MESSAGE|PIPE_WAIT,
						PIPE_UNLIMITED_INSTANCES,
						MyOverlapped::BUFSIZE,
						MyOverlapped::BUFSIZE,
						0,
						NULL);
		if( INVALID_HANDLE_VALUE == hPipe )
		{
			_ftprintf_s(stderr,TEXT("CreateNamedPipe failed, GLE=%d.\n"), GetLastError()); 
		}
		
		OnConnect onConnect(hPipe, m_work);

		m_pio = pPool->newIoCompletion(hPipe, IIoCompletion::FuncPtr());
		m_pio->setIoComplete(IIoCompletion::FuncPtr(onConnect));
		pPool->StartThreadpoolIo(m_pio.get());
			
		BOOL fConnected = ConnectNamedPipe(hPipe, m_olp.get());	// asynchronous ConnectNamedPipe() call - this call will return immediately
															// and the Io completion routine will be called when a client connects.
		if( !fConnected )
		{
			if( GetLastError() == ERROR_IO_PENDING || GetLastError() == ERROR_PIPE_CONNECTED )
				fConnected = TRUE;
		}
		if( !fConnected )
		{
			pPool->CancelThreadpoolIo(m_pio.get());	// prevent a memory leak if ConnectNamedPipe() fails.
		}
	}
}

OnConnect::OnConnect(HANDLE h, const IWorkPtr &pWork) :
m_hPipe(h),
m_work(pWork)
{
}

void
OnConnect::operator()(PTP_CALLBACK_INSTANCE , PVOID Overlapped, ULONG IoResult, ULONG_PTR, IIoCompletion *pIo)
{
	MyOverlapped *pOlp = reinterpret_cast<MyOverlapped *>(Overlapped);
	if( pIo && (NO_ERROR == IoResult) && pOlp)
	{		
		IThreadPool *pPool = pIo->pool();
		pOlp->buffer = pOlp->buffer1;
		RtlGenRandom(pOlp->buffer, MyOverlapped::BUFSIZE);

		::std::shared_ptr<WriteData> write(new WriteData(m_hPipe, pIo, pOlp->shared_from_this(), pPool));
		if( write )
		{
			IWorkPtr myWork = m_work.lock();
			IWorkPtr pWriteDataToPipe = pPool->newWork(IWork::FuncPtr());
			if( pWriteDataToPipe.get() )
			{
				write->setWork(pWriteDataToPipe);
				// ::std::shared_ptr<WriteData> ptr2(write);
				write->setPointer(write);
				pWriteDataToPipe->setWork(IWork::FuncPtr(*write));
				//write work request lambda

				// kickWrite IoCompletionPort lambda
				pIo->setIoComplete(IIoCompletion::FuncPtr(KickWrite(pWriteDataToPipe)));
				pPool->SubmitThreadpoolWork(pWriteDataToPipe.get());
			}

			
			if( myWork )
			{
				// start a new listening session
				pPool->SubmitThreadpoolWork(myWork.get());
			}
		}
	}

}

WriteData::WriteData(HANDLE h, IIoCompletion *pio, const MyOverlappedPtr &olp, IThreadPool * /*pPool*/) : hPipe(h), m_pio(::std::static_pointer_cast<IIoCompletion>(pio->shared_from_this())), m_olp(olp)
{
	//if( pPool )
	//{
	//	IWorkPtr p = pPool->newWork(IWork::FuncPtr(*this));
	//	setWork(p);
	//}
}

void
WriteData::operator()(PTP_CALLBACK_INSTANCE , IWork * /*pWork*/)
{
	bool bKeepWriting = true;
	DWORD cbReplyBytes = MyOverlapped::BUFSIZE;
	DWORD cbWritten = 0;

	unsigned __int8 *pWritePtr = m_olp->buffer;
	m_olp->swapBuffers();
	unsigned __int8 *pGeneratePtr = m_olp->buffer;

	if( m_pio.get() )
	{
		m_pio->pool()->StartThreadpoolIo(m_pio.get());
		// Write the reply to the pipe. 
		BOOL fSuccess = WriteFile( 
					hPipe,        // handle to pipe 
					pWritePtr,     // buffer to write from 
					cbReplyBytes, // number of bytes to write 
					&cbWritten,   // number of bytes written 
					m_olp.get());        // overlapped I/O 

		if(!fSuccess)
		{
			if( ERROR_IO_PENDING == GetLastError() )
				fSuccess = TRUE;
		}

		if (!fSuccess /*|| cbReplyBytes != cbWritten*/ )
		{   
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
	if( !bKeepWriting && m_work )
	{
		::std::shared_ptr<WriteData> ptr(this->shared_from_this());
		m_work->setWork(IWork::FuncPtr([](PTP_CALLBACK_INSTANCE , IWork *){}));
		m_work.reset();
		//release();
	}
}

KickWrite::KickWrite(const IWorkPtr &pWork) : m_work(pWork)
{
}

void
KickWrite::operator()(PTP_CALLBACK_INSTANCE , PVOID, ULONG IoResult, ULONG_PTR, IIoCompletion *pIo)
{
	if( NO_ERROR == IoResult && !m_work.expired() )
	{
		IWorkPtr pWork = m_work.lock();
		if( pWork.get() )
		{
			pIo->pool()->SubmitThreadpoolWork(pWork.get());
		}
	}
}


bool startPipeServer(LPCTSTR lpszPipeName, IThreadPool *pPool)
{
	bool bRetVal = false;
	if( pPool )
	{
		::std::shared_ptr<Connect> pConnect(new Connect(lpszPipeName, pPool));
		if( pConnect )
		{
			pConnect->setPointer(pConnect);
			IWorkPtr pWork = pPool->newWork(IWork::FuncPtr());
			pConnect->setWork(pWork);
			pWork->setWork(IWork::FuncPtr(*pConnect));
			bRetVal = pPool->SubmitThreadpoolWork(pConnect->getWork().get());
		}
	}
	return bRetVal;
}
#endif

#ifdef NEVER

auto noio = [=](PTP_CALLBACK_INSTANCE , PVOID Overlapped, ULONG, ULONG_PTR, IIoCompletion * /* pIo */ ) {};

IWorkPtr makePipeServer(LPCTSTR lpszPipeName, IThreadPool *pPool)
{
	IWorkPtr pWorkPacket = pPool->newWork([](PTP_CALLBACK_INSTANCE , IWork *){});
	::std::shared_ptr<MyOverlapped> olp(new MyOverlapped);

#if 4
	auto kickWrite = [=](PTP_CALLBACK_INSTANCE , PVOID, ULONG IoResult, ULONG_PTR, IIoCompletion *pIo) {
		if( NO_ERROR == IoResult )
		{
			if( pWorkPacket.get() )
			{
				pIo->pool()->SubmitThreadpoolWork(pWorkPacket.get());
			}
		}
		else
			const_cast<IWorkPtr &>(pWorkPacket).reset();	// release our hold on the work item
	};
#endif

#if 2
	auto beginIo = [=](PTP_CALLBACK_INSTANCE , PVOID, ULONG IoResult, ULONG_PTR, IIoCompletion *pIo) {
		if( pIo && (NO_ERROR == IoResult) )
		{
			// IIoCompletionPtr pio2 = pio;
			// HANDLE h = hPipe;

			::std::shared_ptr<MyOverlapped> olp2(olp);

			olp->buffer = olp->buffer1;
			RtlGenRandom(olp->buffer, MyOverlapped::BUFSIZE);
			//write work request lambda
			IWorkPtr pItem2 = pIo->pool()->newWork(write);
						
			if( pItem2.get() )
			{
				// kickWrite IoCompletionPort lambda
				pIo->setIoComplete(kickWrite);
				pItem2->pool()->SubmitThreadpoolWork(pItem2.get());
				// start a new listening session
				pIo->pool()->SubmitThreadpoolWork(pServerItem.get());

			}
			else
			{
				// release our hold on these pointers 
				// we're exiting
				const_cast<IWorkPtr &>(pItem).reset();
				const_cast<IIoCompletionPtr &>(pio).reset();
			}
		}
	};
#endif



	// connect work request lambda
#if 1
	auto connect = [=](PTP_CALLBACK_INSTANCE , IWork *pWork) {
		if( pWork )
		{
			IWorkPtr pItem(::std::dynamic_pointer_cast<IWork>(pWork->shared_from_this()));
			
			::std::shared_ptr<MyOverlapped> olp(new MyOverlapped);
			memset(olp.get(),'\0',sizeof(MyOverlapped));

			HANDLE hPipe = CreateNamedPipe(lpszPipeName,
							PIPE_ACCESS_OUTBOUND|FILE_FLAG_OVERLAPPED,
							PIPE_TYPE_MESSAGE|PIPE_WAIT,
							PIPE_UNLIMITED_INSTANCES,
							MyOverlapped::BUFSIZE,
							MyOverlapped::BUFSIZE,
							0,
							NULL);
			if( INVALID_HANDLE_VALUE == hPipe )
			{
				_ftprintf_s(stderr,TEXT("CreateNamedPipe failed, GLE=%d.\n"), GetLastError()); 
			}
		
			// initialize it with a donothing function. 
			IIoCompletionPtr pio = pPool->newIoCompletion(hPipe, noio);

#if 3
			auto write = [=](PTP_CALLBACK_INSTANCE , IWork * /*pWork*/) {

				DWORD cbReplyBytes = MyOverlapped::BUFSIZE;
				DWORD cbWritten = 0;

				unsigned __int8 *pWritePtr = olp->buffer;
				if( olp->buffer == olp->buffer1 )
					olp->buffer = olp->buffer2;
				else
					olp->buffer = olp->buffer1;
				unsigned __int8 *pGeneratePtr = olp->buffer;

				if( pio.get() )
					pio->pool()->StartThreadpoolIo(pio2.get());
				// Write the reply to the pipe. 
				BOOL fSuccess = WriteFile( 
							hPipe,        // handle to pipe 
							pWritePtr,     // buffer to write from 
							cbReplyBytes, // number of bytes to write 
							&cbWritten,   // number of bytes written 
							olp.get());        // overlapped I/O 

				if(!fSuccess)
				{
					if( ERROR_IO_PENDING == GetLastError() )
						fSuccess = TRUE;
				}
				if (!fSuccess /*|| cbReplyBytes != cbWritten*/ )
				{   
					_ftprintf_s(stderr,TEXT("makePipeServer::write failed, GLE=%d.\n"), GetLastError());
				}
				else if( FALSE == RtlGenRandom(pGeneratePtr, MyOverlapped::BUFSIZE) )
				{
					pio->pool()->CancelThreadpoolIo(pio.get());
					const_cast<IIoCompletionPtr &>(pio).reset();	// release our hold on the IoCompletion object
				}
			};
#endif


			// beginIo IoCompletionPort lambda
#if 2
			auto beginIo = [=](PTP_CALLBACK_INSTANCE , PVOID, ULONG IoResult, ULONG_PTR, IIoCompletion *pIo) {
				IWorkPtr pServerItem(pItem);
				if( pIo && (NO_ERROR == IoResult) )
				{
					IIoCompletionPtr pio2 = pio;
					HANDLE h = hPipe;

					::std::shared_ptr<MyOverlapped> olp2(olp);

					olp->buffer = olp->buffer1;
					RtlGenRandom(olp->buffer, MyOverlapped::BUFSIZE);
					//write work request lambda
#if 3
					auto write = [=](PTP_CALLBACK_INSTANCE , IWork * /*pWork*/) {

						DWORD cbReplyBytes = MyOverlapped::BUFSIZE;
						DWORD cbWritten = 0;

						unsigned __int8 *pWritePtr = olp2->buffer;
						if( olp2->buffer == olp2->buffer1 )
							olp2->buffer = olp2->buffer2;
						else
							olp2->buffer = olp2->buffer1;
						unsigned __int8 *pGeneratePtr = olp2->buffer;

						if( pio2.get() )
							pio2->pool()->StartThreadpoolIo(pio2.get());
						// Write the reply to the pipe. 
						BOOL fSuccess = WriteFile( 
									h,        // handle to pipe 
									pWritePtr,     // buffer to write from 
									cbReplyBytes, // number of bytes to write 
									&cbWritten,   // number of bytes written 
									olp2.get());        // overlapped I/O 

						if(!fSuccess)
						{
							if( ERROR_IO_PENDING == GetLastError() )
								fSuccess = TRUE;
						}
						if (!fSuccess /*|| cbReplyBytes != cbWritten*/ )
						{   
							_ftprintf_s(stderr,TEXT("makePipeServer::write failed, GLE=%d.\n"), GetLastError());
						}
						else if( FALSE == RtlGenRandom(pGeneratePtr, MyOverlapped::BUFSIZE) )
						{
							pio2->pool()->CancelThreadpoolIo(pio2.get());
							const_cast<IIoCompletionPtr &>(pio2).reset();	// release our hold on the IoCompletion object
						}
					};
#endif
					IWorkPtr pItem2 = pIo->pool()->newWork(write);
						
					if( pItem2.get() )
					{
						// kickWrite IoCompletionPort lambda
#if 4
						auto kickWrite = [=](PTP_CALLBACK_INSTANCE , PVOID, ULONG IoResult, ULONG_PTR, IIoCompletion *pIo) {
							if( NO_ERROR == IoResult )
							{
								if( pItem2.get() )
								{
									pIo->pool()->SubmitThreadpoolWork(pItem2.get());
								}
							}
							else
								const_cast<IWorkPtr &>(pItem2).reset();	// release our hold on the work item
						};
#endif
						pio->setIoComplete(kickWrite);
						pItem2->pool()->SubmitThreadpoolWork(pItem2.get());
						// start a new listening session
						pServerItem->pool()->SubmitThreadpoolWork(pServerItem.get());

					}
					else
					{
						// release our hold on these pointers 
						// we're exiting
						const_cast<IWorkPtr &>(pItem).reset();
						const_cast<IIoCompletionPtr &>(pio).reset();
					}
				}
			};
#endif

			pio->setIoComplete(beginIo);
			pPool->StartThreadpoolIo(pio.get());
			
			//IIoCompletionPtr pio = pWork->pool()->newIoCompletion();

			BOOL fConnected = ConnectNamedPipe(hPipe, olp.get());	// asynchronous ConnectNamedPipe() call - this call will return immediately
																// and the Io completion routine will be called when a client connects.
			if( !fConnected )
			{
				if( GetLastError() == ERROR_IO_PENDING || GetLastError() == ERROR_PIPE_CONNECTED )
					fConnected = TRUE;
			}
			if( !fConnected )
			{
				pPool->CancelThreadpoolIo(pio.get());	// prevent a memory leak if ConnectNamedPipe() fails.
			}
		}
	};
#endif

	pWorkPacket->setWork(connect);
	return pWorkPacket;
}



#endif // NEVER