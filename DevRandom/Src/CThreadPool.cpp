#include "stdafx.h"

//#include "RandomDataPipeServer.h"

#include <hash_set>
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW

#endif

#include "IThreadPool.h"
#include "IWork.h"
#include "IWait.h"
// #include "IIoCompletion.h"
#include "IClientConnection.h"
#include "PipeClientConnection.h"
#include "GenericWork.h"
#include "GenericWait.h"
#include "GenericTimer.h"
#include "GenericIoCompletion.h"
#include "SpinLock.h"


template <class C>
class IThreadPoolItemImpl : public C
{
	IThreadPool *m_pool;
public:
	IThreadPoolItemImpl();
	IThreadPoolItemImpl(class CThreadPool *p);
	~IThreadPoolItemImpl();
	virtual IThreadPool *pool()
	{
		return m_pool;
	}
protected:
	friend class CThreadPool;
	virtual void setPool(IThreadPool *p);
};



template <class C>
class IIoCompletionImpl : public C
{
	PTP_IO m_pio;
public:
	virtual PTP_IO pio()
	{
		return m_pio;
	}
	IIoCompletionImpl()
	{
	}
	virtual void setPio(PTP_IO p)
	{
		m_pio = p;
	}
	void setIoComplete(const IIoCompletion::FuncPtr &f)
	{
		__if_exists(C::DoSetIoComplete) {
			C::DoSetIoComplete(f);
		}
		__if_not_exists(C::DoSetIoComplete) {
		UNREFERENCED_PARAMETER(f);
		}
	}
};


/* 

// static
VOID
CALLBACK
IIoCompletion::callback(__inout      PTP_CALLBACK_INSTANCE Instance,
						__inout_opt  PVOID Context,
						__inout_opt  PVOID Overlapped,
						__in         ULONG IoResult,
						__in         ULONG_PTR NumberOfBytesTransferred,
						__inout      PTP_IO Io)
{
	if( 0 != Context )
	{
		BOOL bRetVal = ((IIoCompletion *)Context)->OnComplete(Instance, Overlapped, IoResult, NumberOfBytesTransferred, Io );
		if( !bRetVal )
			delete ((IIoCompletion *)Context);
	}
}

*/

class IWorkImplPrivate
{
public:
	virtual void setPtp(PTP_WORK w)=0;
};

template <class C>
class IWorkImpl : public C, public IWorkImplPrivate
{
	PTP_WORK m_ptpWork;
public:
	IWorkImpl()
	{
		//TRACE(TEXT(">>>IWorkImpl::IWorkImpl<%S>(), this=0x%x.\n"), typeid(C).name() , this); 
	}
	~IWorkImpl()
	{
		// TRACE(TEXT("IWorkImpl::~IWorkImpl<%S>(), this=0x%x.\n"), typeid(C).name() , this); 
	}
	virtual void setWork(const IWork::FuncPtr &f)
	{
		__if_exists(C::doSetWork) {
			C::doSetWork(f);
		}
		__if_not_exists(C::doSetWork) {
			UNREFERENCED_PARAMETER(f);
		}
	}
	virtual PTP_WORK handle() { return m_ptpWork; }
	virtual void setPtp(PTP_WORK w) { m_ptpWork = w; }
};


template <class C>
class IWaitImpl : public C
{
	PTP_WAIT m_ptpWait;
public:
	IWaitImpl()
	{
		//TRACE(TEXT(">>>IWorkImpl::IWorkImpl<%S>(), this=0x%x.\n"), typeid(C).name() , this); 
	}
	~IWaitImpl()
	{
		// TRACE(TEXT("IWorkImpl::~IWorkImpl<%S>(), this=0x%x.\n"), typeid(C).name() , this); 
	}
	virtual PTP_WAIT handle() { return m_ptpWait; }
	virtual void setPtp(PTP_WAIT w) { m_ptpWait = w; }
};

template <class C>
class ITimerImpl : public C
{
	PTP_TIMER m_ptpTimer;
public:
	ITimerImpl()
	{
		//TRACE(TEXT(">>>IWorkImpl::IWorkImpl<%S>(), this=0x%x.\n"), typeid(C).name() , this); 
	}
	~ITimerImpl()
	{
		// TRACE(TEXT("IWorkImpl::~IWorkImpl<%S>(), this=0x%x.\n"), typeid(C).name() , this); 
	}
	virtual PTP_TIMER handle() { return m_ptpTimer; }
	virtual void setPtp(PTP_TIMER t) { m_ptpTimer = t; }
};

class IClientConnectionPrivate
{
public:
	virtual void setPio(PTP_IO pio)=0;
};

template <class C>
class IClientConnectionImpl : public C, public IClientConnectionPrivate
{
	PTP_IO m_pio;
public:
	IClientConnectionImpl() : m_pio(NULL)
	{
	}

	virtual PTP_IO pio()
	{
		return m_pio;
	}
	
	virtual void setPio(PTP_IO pio)
	{
		m_pio = pio;
	}

	virtual HANDLE handle()
	{
		__if_exists(C::handle) {
			return C::handle();
		}
		__if_not_exists(C::handle) {
			return NULL;
		}
	}
};

#ifdef DEPRECATED

class ListenForNewConnection : public IWork
{
	IClientConnection *pConn;
public:
	ListenForNewConnection() : pConn(NULL) {}
	void setConnection(IClientConnection *p) { pConn = p; }
	bool Execute(PTP_CALLBACK_INSTANCE /*Instance*/)
	{
		bool bSucesss = FALSE;
		if( pConn )
		{
			bSucesss = pConn->ListenForNewConnection();
			if( !bSucesss )
			{
				pool()->deleteClientConnectionItem(pConn);
			}
		}
		return false;
	}
};

class WriteRandomData3: public IWork
{
	IClientConnection *pConn;
	enum { BUFSIZE=256 };
	unsigned char buffer[BUFSIZE];
public:
	void setConnection(IClientConnection *pCn) { pConn = pCn; }
	WriteRandomData3() : pConn(NULL)
	{
		RtlGenRandom(buffer, sizeof(buffer));
	}
	~WriteRandomData3()
	{
	}
	bool Execute(PTP_CALLBACK_INSTANCE /*Instance*/)
	{
		bool bRetVal = false;
		__try
		{
			DWORD cbReplyBytes = BUFSIZE;
			DWORD cbWritten = 0;
			if( pConn )
				pool()->StartThreadpoolIo(pConn);
			// Write the reply to the pipe. 
			BOOL fSuccess = WriteFile( 
						pConn->handle(),        // handle to pipe 
						buffer,     // buffer to write from 
						cbReplyBytes, // number of bytes to write 
						&cbWritten,   // number of bytes written 
						pConn->overlapped());        // overlapped I/O 

			if(!fSuccess)
			{
				if( ERROR_IO_PENDING == GetLastError() )
					fSuccess = TRUE;
			}
			if (!fSuccess /*|| cbReplyBytes != cbWritten*/ )
			{   
				_ftprintf_s(stderr,TEXT("WriteRandomData3::WriteFile() failed, GLE=%d.\n"), GetLastError());
				pool()->deleteClientConnectionItem(pConn);
			}
			else
				bRetVal = true;
		
			if( bRetVal )
			{
				bRetVal = (FALSE != RtlGenRandom(buffer, sizeof(buffer)));
				if( !pConn->overlapped() )
					pool()->SubmitThreadpoolWork(this);
			}
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			bRetVal = false;
		}
		return bRetVal;
	}
};
#endif // DEPRECATED


class CThreadPool : public IThreadPool
{
	TP_CALLBACK_ENVIRON m_env;
	unsigned __int32	m_bEnabled;
	::std::hash_set<IThreadPoolItem *> m_items;
	SpinLock		m_lock;
public:
	void insertItem(IThreadPoolItem *pItem)
	{
		// protect access to the hash table
		m_lock.acquire();
		m_items.insert(pItem);
		m_lock.release();
	}
	void removeItem(IThreadPoolItem *pItem)
	{
		m_lock.acquire();
		m_items.erase(pItem);
		m_lock.release();
	}
private:
	DWORD GetThreadPoolSize()
	{
		SYSTEM_INFO si = {0};
		GetSystemInfo(&si);
	
		DWORD dwNumCpu = si.dwNumberOfProcessors - 1;
		if( 0 == dwNumCpu )
			dwNumCpu = 1;
	
		return dwNumCpu;
	}

	virtual bool Enabled()
	{
		return (InterlockedCompareExchange(&m_bEnabled,TRUE,TRUE) == TRUE);
	}

	virtual void Shutdown()
	{
		InterlockedCompareExchange(&m_bEnabled,FALSE,TRUE);
		CloseThreadpoolCleanupGroupMembers(env()->CleanupGroup, TRUE, NULL);
		CloseThreadpoolCleanupGroup(env()->CleanupGroup);
		env()->CleanupGroup = NULL;
		PTP_POOL pool = env()->Pool;
		DestroyThreadpoolEnvironment(&m_env);
		memset(&m_env, '\0', sizeof(m_env));
		CloseThreadpool(pool);
		
		if( m_items.size() > 0 )
		{
			::std::vector<IThreadPoolItem *>theList;
			theList.reserve(m_items.size());
			theList.insert(theList.begin(),m_items.begin(), m_items.end());
			::std::for_each(theList.begin(), theList.end(), [&](IThreadPoolItem *pItem) {
				if( pItem )
				{
					delete pItem;
				}
			});
		}
	}

	virtual void deleteWorkItem(IWork *&pWork)
	{
		if( pWork )
		{
			// WaitForThreadpoolWorkCallbacks(pWork->handle(), TRUE);
			CloseThreadpoolWork(pWork);
			delete pWork;
			pWork = NULL;
		}
	}

	virtual void deleteClientConnectionItem(IClientConnection *&pCn)
	{
		if( pCn )
		{
			// ::WaitForThreadpoolIoCallbacks(pCn->pio(), TRUE);
			//DWORD nXfered = TRUE;
			//BOOL bResults = GetOverlappedResult(pCn->handle(), pCn->overlapped(), &nXfered, FALSE);
			//if( !bResults )
			//{
			//	_ftprintf_s(stderr,TEXT("GetOverlappedResult failed, GLE=%d.\n"), GetLastError()); 
			//}
			::CloseThreadpoolIo(pCn->pio());
			IClientConnectionPrivate *ptr = dynamic_cast<IClientConnectionPrivate *>(pCn);
			if( ptr )
				ptr->setPio(NULL);
			delete pCn;
			pCn = NULL;
			
		}
	}

	bool CloseThreadpoolWork(class IWork *work)
	{
		bool bRetVal = (NULL != work) && Enabled();
		if( bRetVal )
		{
			::CloseThreadpoolWork(work->handle());
			IWorkImplPrivate *ptr = dynamic_cast<IWorkImplPrivate *>(work);
			if( ptr )
				ptr->setPtp(NULL);
		}

		return bRetVal;
	}

	bool SetThreadpoolWait(class IWait *wait, HANDLE h, PFILETIME pftTimeout)
	{
		bool bRetVal = (NULL != wait) && Enabled();
		if( bRetVal )
			::SetThreadpoolWait(wait->handle(),h,pftTimeout);
		return bRetVal;
	}

	bool SubmitThreadpoolWork(class IWork *work)
	{
		bool bRetVal = (NULL != work) && Enabled();
		if( bRetVal )
			::SubmitThreadpoolWork(work->handle());
		return bRetVal;
	}

	virtual bool StartThreadpoolIo(class IIoCompletion *pIo)
	{
		bool bRetVal = (NULL != pIo) && Enabled() && (NULL != pIo->pio());
		if( bRetVal )
			::StartThreadpoolIo(pIo->pio());
		return bRetVal;
	}

	virtual bool CancelThreadpoolIo(class IIoCompletion *pIo)
	{
		bool bRetVal = (pIo && Enabled());
		if( bRetVal )
			::CancelThreadpoolIo(pIo->pio());
		return bRetVal;
	}

	virtual bool CloseThreadpoolIo(class IIoCompletion *pIo)
	{
		bool bRetVal = (pIo && Enabled());
		if( bRetVal )
			::CloseThreadpoolIo(pIo->pio());
		return bRetVal;
	}

	virtual void WaitForThreadpoolIoCallbacks(class IClientConnection *pCn, BOOL bCancelPending)
	{
		if( pCn )
			::WaitForThreadpoolIoCallbacks(pCn->pio(), bCancelPending);
	}

public:
	CThreadPool(const IThreadPool::Priority priority=IThreadPool::NORMAL, DWORD dwMinThreads=0, DWORD dwMaxThreads=0) :
	 m_bEnabled(FALSE),
	m_lock(false)
	{
		PTP_POOL pool =  CreateThreadpool(NULL);
		// PTP_IO pio = CreateThreadpoolIo(

		if( 0 == dwMaxThreads )
			dwMaxThreads = GetThreadPoolSize();
		if( 0 == dwMinThreads )
			dwMinThreads = 1;

		_ftprintf_s(stdout, TEXT("Creating Thread Pool with %d max threads\n"), dwMaxThreads);

		SetThreadpoolThreadMinimum(pool, dwMinThreads);
		SetThreadpoolThreadMaximum(pool, dwMaxThreads);

		InitializeThreadpoolEnvironment(env());
		//SetThreadpoolCallbackRunsLong(&env);

	
		
		SetThreadpoolCallbackPool(env(), pool);

		PTP_CLEANUP_GROUP cleanup_group = CreateThreadpoolCleanupGroup();
		if( !cleanup_group )
		{
			_ftprintf_s(stderr,TEXT("CreateThreadpoolCleanupGroup failed, GLE=%d.\n"), GetLastError());
		}
		
		TP_CALLBACK_PRIORITY pri = TP_CALLBACK_PRIORITY_NORMAL;
		switch( priority )
		{
			case LOW:
				pri = TP_CALLBACK_PRIORITY_LOW;
				break;
			case HIGH:
				pri = TP_CALLBACK_PRIORITY_HIGH;
				break;
			case NORMAL:
			default:
				pri = TP_CALLBACK_PRIORITY_NORMAL;
				break;
		}


		SetThreadpoolCallbackPriority(env(), pri);


		SetThreadpoolCallbackCleanupGroup(env(), cleanup_group, NULL);

		InterlockedCompareExchange(&m_bEnabled,TRUE,FALSE);
		
	}
	~CThreadPool()
	{
		InterlockedCompareExchange(&m_bEnabled,FALSE,TRUE);

		if( env()->CleanupGroup )
		{
			CloseThreadpoolCleanupGroupMembers(env()->CleanupGroup, TRUE, NULL);
			CloseThreadpoolCleanupGroup(env()->CleanupGroup);
		}
		if( env()->Pool )
		{
			PTP_POOL pool = env()->Pool;
			DestroyThreadpoolEnvironment(env());
			CloseThreadpool(pool);
		}
	}
	virtual PTP_CALLBACK_ENVIRON env() { return &m_env; }
#ifdef DEPRECATED
	virtual IClientConnection *newNamedPipeConnection()
	{
		IClientConnection *pRetVal = NULL;
		if( Enabled() )
			pRetVal = new IThreadPoolItemImpl< IClientConnectionImpl< PipeClientConnection > >(this);

		return pRetVal;
	}
	virtual IWork *newWaitForNewConnection(class IClientConnection *pCn)
	{
		IThreadPoolItemImpl< IWorkImpl<ListenForNewConnection> > *pNewWork = NULL;
		if( Enabled() )
		{
			pNewWork = new IThreadPoolItemImpl< IWorkImpl<ListenForNewConnection> >(this);
			if(  pNewWork )
			{
				createThreadPoolWork(static_cast<IWorkImpl<ListenForNewConnection> *>(pNewWork));
				pNewWork->setConnection(pCn);
			}
		}
		return pNewWork;
	}
	virtual IWork * newWriteRandomData3(IClientConnection *pCn)
	{
		IThreadPoolItemImpl< IWorkImpl<WriteRandomData3> > *pData = NULL;
		if( Enabled() )
		{
			pData = new IThreadPoolItemImpl< IWorkImpl<WriteRandomData3> >(this);
			if( pData )
			{
				createThreadPoolWork(static_cast<IWorkImpl<WriteRandomData3> *>(pData));
				pData->setConnection(pCn);
			}
		}
		return pData;
	}
#endif // DEPRECATED
	virtual IWorkPtr newWork(const IWork::FuncPtr &f)
	{
		::std::shared_ptr<IWorkImpl<IThreadPoolItemImpl<GenericWork> > >  ptr = ::std::make_shared<IWorkImpl<IThreadPoolItemImpl<GenericWork> > >();
		if( ptr )
		{
			ptr->setPool(this);
			ptr->setWork(f);
			createThreadPoolWork(ptr.get());
		}
		return ::std::static_pointer_cast<IWork>(ptr);
	}

	virtual IWaitPtr newWait(const IWait::FuncPtr &f)
	{
		::std::shared_ptr<IWaitImpl<IThreadPoolItemImpl<GenericWait> > >  ptr = ::std::make_shared<IWaitImpl<IThreadPoolItemImpl<GenericWait> > >();
		if( ptr )
		{
			ptr->setPool(this);
			ptr->setWait(f);
			createThreadPoolWait(ptr.get());
		}
		return ::std::static_pointer_cast<IWait>(ptr);
	}

	virtual ITimerPtr newTimer(const ITimer::FuncPtr &f)
	{
		::std::shared_ptr<ITimerImpl<IThreadPoolItemImpl<GenericTimer> > >  ptr = ::std::make_shared<ITimerImpl<IThreadPoolItemImpl<GenericTimer> > >();
		if( ptr )
		{
			ptr->setPool(this);
			ptr->setTimer(f);
			createThreadPoolTimer(ptr.get());
		}
		return ::std::static_pointer_cast<ITimer>(ptr);
	}


	virtual IIoCompletionPtr newIoCompletion(HANDLE hIoObject, const IIoCompletion::FuncPtr &f)
	{
		::std::shared_ptr<IIoCompletionImpl<IThreadPoolItemImpl<GenericIoCompletion> > >  ptr = ::std::make_shared<IIoCompletionImpl<IThreadPoolItemImpl<GenericIoCompletion> > >();
		if( ptr )
		{
			ptr->setPool(this);
			ptr->setIoComplete(f);
			createThreadPoolIoCompletion(hIoObject, ptr.get());
		}
		return ::std::static_pointer_cast<IIoCompletion>(ptr);
	}

	template <class C>
	void createThreadPoolWork(IWorkImpl<C> *pWork)
	{
		if( pWork && Enabled())
		{
			PVOID Param = reinterpret_cast<PVOID>(static_cast<C *>(pWork));
			PTP_WORK ptpWork = ::CreateThreadpoolWork(IWork_callback, Param, env());
		
			if( NULL == ptpWork )
			{
				_ftprintf_s(stderr,TEXT("CreateThreadpoolWork failed, GLE=%d.\n"), GetLastError()); 
			}
			pWork->setPtp(ptpWork);
		}
	}

	template <class C>
	void createThreadPoolWait(IWaitImpl<C> *pWait)
	{
		if( pWait && Enabled())
		{
			PTP_WAIT ptpWait = ::CreateThreadpoolWait(IWait_callback, reinterpret_cast<PVOID>(pWait), env());
		
			if( NULL == ptpWait )
			{
				_ftprintf_s(stderr,TEXT("CreateThreadpoolWait failed, GLE=%d.\n"), GetLastError()); 
			}
			pWait->setPtp(ptpWait);
		}
	}

	template <class C>
	void createThreadPoolTimer(ITimerImpl<C> *pTimer)
	{
		if( pTimer && Enabled())
		{
			PTP_TIMER ptpTimer = ::CreateThreadpoolTimer(ITimer_callback, reinterpret_cast<PVOID>(pTimer), env());
		
			if( NULL == ptpTimer )
			{
				_ftprintf_s(stderr,TEXT("CreateThreadpoolTimer failed, GLE=%d.\n"), GetLastError()); 
			}
			pTimer->setPtp(ptpTimer);
		}
	}

	template <class C>
	void createThreadPoolIoCompletion(HANDLE hIo, IIoCompletionImpl<C> *pIoCompletion)
	{
		if( pIoCompletion && Enabled() )
		{
			PTP_IO pio = ::CreateThreadpoolIo(hIo, IIoCompletion_callback, (PVOID)(IIoCompletion *)pIoCompletion, env());
			if( pio )
				pIoCompletion->setPio(pio);
		}
	}

	static VOID CALLBACK IWork_callback(PTP_CALLBACK_INSTANCE Instance, PVOID Context, PTP_WORK /*Work*/)
	{
		IWork *pWork = reinterpret_cast<IWork *>(Context);
		if( pWork )
		{
			if( pWork->pool()->Enabled() )
				pWork->Execute(Instance);
		}
#ifdef DEPRECATED
		if(  !bSuccess && pWork )
			pWork->pool()->deleteWorkItem(pWork);
#endif // DEPRECATED
	}

	static VOID CALLBACK IWait_callback(PTP_CALLBACK_INSTANCE Instance, PVOID Context, PTP_WAIT /*Wait*/, TP_WAIT_RESULT WaitResult)
	{
		IWait *pWait = reinterpret_cast<IWait *>(Context);
		if( pWait )
		{
			if( pWait->pool()->Enabled() )
				pWait->Execute(Instance,WaitResult);
		}
	}

	static VOID CALLBACK ITimer_callback(PTP_CALLBACK_INSTANCE Instance, PVOID Context, PTP_TIMER /*Wait*/)
	{
		ITimer *pTimer = reinterpret_cast<ITimer *>(Context);
		if( pTimer )
		{
			if( pTimer->pool()->Enabled() )
				pTimer->Execute(Instance);
		}
	}

	static VOID CALLBACK IClientConnection_callback(__inout      PTP_CALLBACK_INSTANCE Instance,
													__inout_opt  PVOID Context,
													__inout_opt  PVOID Overlapped,
													__in         ULONG IoResult,
													__in         ULONG_PTR NumberOfBytesTransferred,
													__inout      PTP_IO Io)
	{
		if( 0 != Context )
		{
			IClientConnection *pConn = reinterpret_cast<IClientConnection *>(Context);
			if( pConn->pool()->Enabled() )
				pConn->OnComplete(Instance, Overlapped, IoResult, NumberOfBytesTransferred, Io );
#ifdef DEPRECATED
			if( !bRetVal )
			{
				pConn->pool()->deleteClientConnectionItem(pConn);
			}
#endif // DEPRECATED
		}
	}

	static VOID CALLBACK IIoCompletion_callback(__inout      PTP_CALLBACK_INSTANCE Instance,
													__inout_opt  PVOID Context,
													__inout_opt  PVOID Overlapped,
													__in         ULONG IoResult,
													__in         ULONG_PTR NumberOfBytesTransferred,
													__inout      PTP_IO /*Io*/)
	{
		if( 0 != Context )
		{
			IIoCompletion *pIo = reinterpret_cast<IIoCompletion *>(Context);
			if( pIo->pool()->Enabled() )
				pIo->OnComplete(Instance, Overlapped, IoResult, NumberOfBytesTransferred );
		}
	}

};

template<class C>
IThreadPoolItemImpl<C>::IThreadPoolItemImpl() : m_pool(NULL)
{
	TRACE(TEXT(">>>%S::IThreadPoolItemImpl(), this=0x%x.\n"), typeid(*this).name() , this); 
}

template<class C>
IThreadPoolItemImpl<C>::IThreadPoolItemImpl(class CThreadPool *p) : m_pool(p)
{
	TRACE(TEXT(">>>%S::IThreadPoolItemImpl(p), this=0x%x.\n"), typeid(*this).name() , this); 
	p->insertItem(this);
}
template<class C>
IThreadPoolItemImpl<C>::~IThreadPoolItemImpl()
{
	TRACE(TEXT("<<<%S::~IThreadPoolItemImpl(), this=0x%x.\n"), typeid(*this).name() , this); 
	static_cast<CThreadPool *>(pool())->removeItem(this);
}
template<class C>
void
IThreadPoolItemImpl<C>::setPool(IThreadPool *p)
{
	static_cast<CThreadPool *>(p)->insertItem(this);
	m_pool = p;
}

::std::shared_ptr<IThreadPool>
IThreadPool::newPool(const IThreadPool::Priority priority, const DWORD dwMinThreads, const DWORD dwMaxThreads)
{
	return ::std::static_pointer_cast<IThreadPool>(::std::make_shared<CThreadPool>(priority, dwMinThreads, dwMaxThreads));
}