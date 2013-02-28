#pragma once
#ifndef __ITHREADPOOL_H__
#define __ITHREADPOOL_H__

#include <memory>
#include <functional>
#include "IWork.h"
#include "IWait.h"
#include "ITimer.h"
#include "IIoCompletion.h"


typedef ::std::shared_ptr<class IThreadPool> IThreadPoolPtr;

/// <summary>
/// IThreadPool is the interface to the thread pool. In addition to managing the thread pool itself.
/// this interface is also responsible for creating and destroying the items that interact with the 
/// thead pool.
/// </summary>
class IThreadPool
{
public:
	virtual PTP_CALLBACK_ENVIRON env()=0;
	virtual ~IThreadPool()
	{
	}
	enum Priority 
	{
		NORMAL,
		LOW,
		HIGH
	};

	virtual void Shutdown()=0;
	virtual bool Enabled()=0;

public:
	virtual IWorkPtr			newWork(const IWork::FuncPtr &f)=0;
	virtual IIoCompletionPtr	newIoCompletion(HANDLE hIoObject, const IIoCompletion::FuncPtr &f)=0;
	virtual IWaitPtr			newWait(const IWait::FuncPtr &f)=0;
	virtual ITimerPtr			newTimer(const ITimer::FuncPtr &f)=0;

public:
	/// Close a thread pool work item and release the OS resources.
	virtual bool CloseThreadpoolWork(class IWork *work)=0;
	virtual bool SubmitThreadpoolWork(class IWork *work)=0;
	virtual bool StartThreadpoolIo(class IIoCompletion *pCn)=0;
	virtual bool CancelThreadpoolIo(class IIoCompletion *pCn)=0;
	virtual bool CloseThreadpoolIo(class IIoCompletion *pCn)=0;
	virtual void WaitForThreadpoolIoCallbacks(class IClientConnection *pCn, BOOL bCancelPending)=0;

	// Create a new Thread pool.
	// with the given priority, minimum and maximum number of threads.
	// pass dwMinThreads=0 & dwMaxThreads=0 to automatically choose these values based
	// on your system.
	static IThreadPoolPtr newPool(const IThreadPool::Priority priority=IThreadPool::NORMAL, const DWORD dwMinThreads=0, const DWORD dwMaxThreads=0);
};


#endif // __ITHREADPOOL_H__