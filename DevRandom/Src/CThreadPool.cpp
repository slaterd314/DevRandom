#include "stdafx.h"

#include <hash_set>
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW

#endif

#include "IThreadPool.h"
#include "IWork.h"
// #include "IIoCompletion.h"
#include "IClientConnection.h"
#include "PipeClientConnection.h"
#include "GenericWork.h"


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

/*

template <class C>
class IIoCompletionImpl : public C
{
public:
	IIoCompletionImpl()
	{
	}
};
*/

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
		TRACE(TEXT(">>>IWorkImpl::IWorkImpl<%S>(), this=0x%x.\n"), typeid(C).name() , this); 
	}
	~IWorkImpl()
	{
		TRACE(TEXT("IWorkImpl::~IWorkImpl<%S>(), this=0x%x.\n"), typeid(C).name() , this); 
	}
	virtual PTP_WORK handle() { return m_ptpWork; }
	virtual void setPtp(PTP_WORK w) { m_ptpWork = w; }
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

class CThreadPool : public IThreadPool
{
	TP_CALLBACK_ENVIRON m_env;
	unsigned __int32	m_bEnabled;
	::std::hash_set<IThreadPoolItem *> m_items;
public:
	void insertItem(IThreadPoolItem *pItem)
	{
		m_items.insert(pItem);
	}
	void removeItem(IThreadPoolItem *pItem)
	{
		m_items.erase(pItem);
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

	bool SubmitThreadpoolWork(class IWork *work)
	{
		bool bRetVal = (NULL != work) && Enabled();
		if( bRetVal )
			::SubmitThreadpoolWork(work->handle());
		return bRetVal;
	}

	virtual bool StartThreadpoolIo(class IClientConnection *pCn)
	{
		bool bRetVal = (NULL != pCn) && Enabled() && (NULL != pCn->pio());
		if( bRetVal )
			::StartThreadpoolIo(pCn->pio());
		return bRetVal;
	}

	virtual bool CreateThreadpoolIo(class IClientConnection *pCn)
	{
		bool bRetVal = false;
		if( pCn && Enabled())
		{
			IClientConnectionPrivate *ptr = dynamic_cast<IClientConnectionPrivate *>(pCn);
			if( ptr )
			{
				PTP_IO pio = ::CreateThreadpoolIo(pCn->handle(), IClientConnection_callback, (PVOID)pCn, env());
				if( NULL != pio )
				{
					ptr->setPio(pio);
					bRetVal = true;
				}
			}
		}
		return bRetVal;
	}
	virtual bool CancelThreadpoolIo(class IClientConnection *pCn)
	{
		bool bRetVal = (pCn && Enabled());
		if( bRetVal )
			::CancelThreadpoolIo(pCn->pio());
		return bRetVal;
	}

	virtual void WaitForThreadpoolIoCallbacks(class IClientConnection *pCn, BOOL bCancelPending)
	{
		if( pCn )
			::WaitForThreadpoolIoCallbacks(pCn->pio(), bCancelPending);
	}

public:
	CThreadPool(const IThreadPool::Priority priority=IThreadPool::NORMAL, DWORD dwMinThreads=0, DWORD dwMaxThreads=0) : m_bEnabled(FALSE)
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

	virtual ::std::shared_ptr<IWork> newWork(const ::std::function<bool (PTP_CALLBACK_INSTANCE,IWork *) > &f)
	{
		::std::shared_ptr<IWorkImpl<IThreadPoolItemImpl<GenericWork> > >  ptr = ::std::make_shared<IWorkImpl<IThreadPoolItemImpl<GenericWork> > >();
		if( ptr )
		{
			ptr->setPool(this);
			ptr->setFunc(f);
		}
		return ::std::static_pointer_cast<IWork>(ptr);
	}

	template <class C>
	void createThreadPoolWork(IWorkImpl<C> *pWork)
	{
		if( pWork && Enabled())
		{
			PTP_WORK ptpWork = ::CreateThreadpoolWork(IWork_callback, reinterpret_cast<PVOID>(pWork), env());
		
			if( NULL == ptpWork )
			{
				_ftprintf_s(stderr,TEXT("CreateThreadpoolWork failed, GLE=%d.\n"), GetLastError()); 
			}
			pWork->setPtp(ptpWork);
		}
	}

	static VOID CALLBACK IWork_callback(PTP_CALLBACK_INSTANCE Instance, PVOID Context, PTP_WORK /*Work*/)
	{
		IWork *pWork = reinterpret_cast<IWork *>(Context);
		bool bSuccess = FALSE;
		if( pWork )
		{
			if( pWork->pool()->Enabled() )
				bSuccess = pWork->Execute(Instance);
		}
		if(  !bSuccess && pWork )
			pWork->pool()->deleteWorkItem(pWork);
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
			BOOL bRetVal = FALSE;
			if( pConn->pool()->Enabled() )
			{
				bRetVal = pConn->OnComplete(Instance, Overlapped, IoResult, NumberOfBytesTransferred, Io );
			}
			if( !bRetVal )
			{
				pConn->pool()->deleteClientConnectionItem(pConn);
			}
		}
	}

};

template<class C>
IThreadPoolItemImpl<C>::IThreadPoolItemImpl() : m_pool(NULL)
{
}

template<class C>
IThreadPoolItemImpl<C>::IThreadPoolItemImpl(class CThreadPool *p) : m_pool(p)
{
	p->insertItem(this);
}
template<class C>
IThreadPoolItemImpl<C>::~IThreadPoolItemImpl()
{
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
createThreadPool(const IThreadPool::Priority priority, const DWORD dwMinThreads, const DWORD dwMaxThreads)
{
	return ::std::static_pointer_cast<IThreadPool>(::std::make_shared<CThreadPool>(priority, dwMinThreads, dwMaxThreads));
}