#pragma once
#ifndef __CTHREADPOOL_H__
#define __CTHREADPOOL_H__

#include "IThreadPool.h"
#include "IWork.h"
#include "IWait.h"
#include "IIoCompletion.h"
#include "ITimer.h"
#include "IThreadPoolItemImpl.h"
#include "IIoCompletionImpl.h"
#include "IWorkImpl.h"
#include "IWaitImpl.h"
#include "ITimerImpl.h"
#include "SpinLock.h"

class CThreadPool : public IThreadPool
{
public:
	void insertItem(IThreadPoolItem *pItem);
	void removeItem(IThreadPoolItem *pItem);
public:
	DWORD GetThreadPoolSize();

	virtual bool Enabled();
	virtual void Shutdown();
	virtual void deleteWorkItem(IWork *&pWork);
	bool CloseThreadpoolWork(IWork *work);
	virtual bool CloseThreadpoolWait(IWait *wait);
	bool SetThreadpoolWait(IWait *wait, HANDLE h, PFILETIME pftTimeout);
	bool SubmitThreadpoolWork(IWork *work);
	virtual bool SetThreadpoolWait(HANDLE h, PFILETIME pftTImeout, IWait *wait);
	virtual bool StartThreadpoolIo(IIoCompletion *pIo);
	virtual bool CancelThreadpoolIo(IIoCompletion *pIo);
	virtual bool CloseThreadpoolIo(IIoCompletion *pIo);
	virtual bool SetThreadpoolTimer(PFILETIME pftDueTime, DWORD msPeriod, DWORD msWindowLength, class ITimer *timer);
	virtual bool CloseThreadpoolTimer(class ITimer *timer);
	virtual void WaitForThreadpoolWorkCallbacks(IWork *work, BOOL bCancelPendingCallbacks);
	virtual void WaitForThreadpoolIoCallbacks(IIoCompletion *pio, BOOL bCancelPendingCallbacks);
	virtual void WaitForThreadpoolWaitCallbacks(IWait *wait, BOOL bCancelPendingCallbacks);
	virtual void WaitForThreadpoolTimerCallbacks(ITimer *timer, BOOL bCancelPendingCallbacks);
	virtual void SetThreadpoolCallbackLibrary( void *mod);
	virtual void SetThreadpoolCallbackRunsLong();
public:
	CThreadPool(const IThreadPool::Priority priority=IThreadPool::NORMAL, DWORD dwMinThreads=0, DWORD dwMaxThreads=0);
	~CThreadPool();
	virtual PTP_CALLBACK_ENVIRON env();

	virtual IWorkPtr newWork(const IWork::FuncPtr &f);
	virtual IWaitPtr newWait(const IWait::FuncPtr &f);
	virtual ITimerPtr newTimer(const ITimer::FuncPtr &f);
	virtual IIoCompletionPtr newIoCompletion(HANDLE hIoObject, const IIoCompletion::FuncPtr &f);
	template <class C>
	bool createThreadPoolWork(IWorkImpl<C> *pWork);
	template <class C>
	bool createThreadPoolWait(IWaitImpl<C> *pWait);
	template <class C>
	bool createThreadPoolTimer(ITimerImpl<C> *pTimer);
	template <class C>
	bool createThreadPoolIoCompletion(HANDLE hIo, IIoCompletionImpl<C> *pIoCompletion);

	static VOID CALLBACK IWork_callback(PTP_CALLBACK_INSTANCE Instance, PVOID Context, PTP_WORK /*Work*/);
	static VOID CALLBACK IWait_callback(PTP_CALLBACK_INSTANCE Instance, PVOID Context, PTP_WAIT /*Wait*/, TP_WAIT_RESULT WaitResult);
	static VOID CALLBACK ITimer_callback(PTP_CALLBACK_INSTANCE Instance, PVOID Context, PTP_TIMER /*Timer*/);
	static VOID CALLBACK IIoCompletion_callback(__inout      PTP_CALLBACK_INSTANCE Instance,
													__inout_opt  PVOID Context,
													__inout_opt  PVOID Overlapped,
													__in         ULONG IoResult,
													__in         ULONG_PTR NumberOfBytesTransferred,
													__inout      PTP_IO /*Io*/);
protected:
	explicit CThreadPool(int);
private:
	TP_CALLBACK_ENVIRON m_env;
	unsigned __int32	m_bEnabled;
	::std::unique_ptr<::std::hash_set<IThreadPoolItem *> > m_items;
	LWSpinLock		m_lock;
};

#endif // __CTHREADPOOL_H__
