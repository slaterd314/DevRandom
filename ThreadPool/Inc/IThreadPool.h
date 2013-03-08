#pragma once
#ifndef __ITHREADPOOL_H__
#define __ITHREADPOOL_H__

#include <memory>
#include <functional>
#include "IWork.h"
#include "IWait.h"
#include "ITimer.h"
#include "IIoCompletion.h"

#ifndef THREADPOOL_API
#define THREADPOOL_API __declspec(dllimport)
#endif

/// <summary>
/// IThreadPool is the interface to the thread pool. In addition to managing the thread pool itself.
/// this interface is also responsible for creating and destroying the items that interact with the 
/// thead pool.
/// </summary>
class IThreadPool
{
public:
	typedef ::std::shared_ptr<class IThreadPool> Ptr;
	/// The callback environment used by this thread pool
	virtual PTP_CALLBACK_ENVIRON env()=0;
	virtual ~IThreadPool()
	{
	}
	/// Thread pool thread priority 
	enum Priority 
	{
		NORMAL,
		LOW,
		HIGH
	};

	/// Shutdown the thread pool
	virtual void Shutdown()=0;
	/// test to see if the thread pool is enabled. If Enabled()
	/// returns false, then the thread pool is likely shutting down.
	virtual bool Enabled()=0;

public:
	/// Create a new IWork object that will execute f when called back.
	virtual IWork::Ptr			newWork(const IWork::FuncPtr &f)=0;
	/// Create a new IoCompletion port object that will execute f when called back.
	virtual IIoCompletion::Ptr	newIoCompletion(HANDLE hIoObject, const IIoCompletion::FuncPtr &f)=0;
	/// Create a new wait object that will execute f when called back.
	virtual IWait::Ptr			newWait(const IWait::FuncPtr &f)=0;
	/// Create a new timer object that will execute f when called back.
	virtual ITimer::Ptr			newTimer(const ITimer::FuncPtr &f)=0;

public:
	/// wrapper around SetThreadpoolCallbackLibrary() API - ensures the dynamic link library
	/// identified by mod ( the module handle ) remains loaded as long as there are outstanding callbacks
	/// in the thread pool.
	virtual void SetThreadpoolCallbackLibrary( void *mod)=0;
	/// notifys the thread pool that callbacks may run for a long time. see MS SetThreadpoolCallbackRunsLong() doc.
	virtual void SetThreadpoolCallbackRunsLong()=0;

	/// Create a new Thread pool.
	/// with the given priority, minimum and maximum number of threads.
	/// pass dwMinThreads=0 & dwMaxThreads=0 to automatically choose these values based
	/// on your system.
	static THREADPOOL_API Ptr newPool(const IThreadPool::Priority priority=IThreadPool::NORMAL, const DWORD dwMinThreads=0, const DWORD dwMaxThreads=0);
	/// retrieve an IThreadPool interface wrapping the processes default thread pool - do not delete the returned pointer
	/// Note: the thread pool returned by this method has no environment - i.e. env() returns false.
	/// as a result, SetThreadpoolCallbackLibrary() and SetThreadpoolCallbackRunsLong() are ineffective for the default
	/// thread pool.
	static THREADPOOL_API IThreadPool *getDefaultPool();
};


#endif // __ITHREADPOOL_H__