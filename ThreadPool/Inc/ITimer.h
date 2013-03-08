#pragma once
#ifndef __ITIMER_H__
#define __ITIMER_H__

#include "IThreadPoolItem.h"

class 
__declspec(novtable)
ITimer : public IThreadPoolItem
{
public:
	virtual ~ITimer(){}
	/// typedef for the function this object uses.
	typedef ::std::function<void (PTP_CALLBACK_INSTANCE, ITimer *)> FuncPtr;
	/// Execute the timer. This method is called from the thread pool timer item callback.
	virtual void Execute(PTP_CALLBACK_INSTANCE)=0;
	/// Assign a new std::function object to be executed by Execute()
	virtual void setTimer(const FuncPtr &f)=0;	
	/// Access to the underlying windows PTP_TIMER handle.
	virtual PTP_TIMER handle()=0;
	/// Queue up this timer object to the thread pool. Execute() will be called 
	/// when either h is signaled, or if the wait times out.
	/// pftTimeout can be NULL to indicate an infinite wait.
	virtual bool SetThreadpoolTimer(PFILETIME pftDueTime, DWORD msPeriod, DWORD msWindowLength)=0;
	/// Close the underlying PTP_TIMER handle. Once this method is called, this object is no longer valid for thread pool calls.
	virtual bool CloseTimer()=0;
	/// Wait for any pending callbacks on this timer object, optionally canceling any timer objects
	/// that are queued but haven't run yet. (bCancelPendingCallbacks=TRUE cancels pending callbacks)
	/// Be careful using this one with bCancelPendingCallbacks == FALSE. In that case, this method will call one of the Wait API's
	/// so calling it from your thread pool threads can deadlock the pool. 
	virtual void WaitForCallbacks(BOOL bCancelPendingCallbacks)=0;
	typedef ::std::shared_ptr<class ITimer> Ptr;
};

#endif // __ITIMER_H__
