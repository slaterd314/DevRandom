#pragma once
#ifndef __IWAIT_H__
#define __IWAIT_H__

#include "IThreadPoolItem.h"

class 
__declspec(novtable)
IWait : public IThreadPoolItem
{
public:
	virtual ~IWait(){}
	/// typedef for the function this object uses.
	typedef ::std::function<void (PTP_CALLBACK_INSTANCE , TP_WAIT_RESULT , IWait *)> FuncPtr;
	/// Execute the wait. This method is called from the thread pool wait item callback.
	virtual void Execute(PTP_CALLBACK_INSTANCE , TP_WAIT_RESULT)=0;
	/// Assign a new std::function object to be executed by Execute()
	virtual void setWait(const FuncPtr &f)=0;
	/// Access to the underlying windows PTP_WAIT handle.
	virtual PTP_WAIT handle()=0;
	/// Queue up this wait object to the thread pool. Execute() will be called 
	/// when either h is signaled, or if the wait times out.
	/// pftTimeout can be NULL to indicate an infinite wait.
	virtual bool SetThreadpoolWait(HANDLE h, PFILETIME pftTimeout)=0;
	/// Close the underlying PTP_WAIT handle. Once this method is called, this object is no longer valid for thread pool calls.
	virtual bool CloseWait()=0;
	/// Wait for any pending callbacks on this wait object, optionally canceling any wait objects
	/// that are queued but haven't run yet. (bCancelPendingCallbacks=TRUE cancels pending callbacks)
	/// Be careful using this one with bCancelPendingCallbacks == FALSE. In that case, this method will call one of the Wait API's
	/// so calling it from your thread pool threads can deadlock the pool. 
	virtual void WaitForCallbacks(BOOL bCancelPendingCallbacks)=0;
	typedef ::std::shared_ptr<class IWait> Ptr;
};

#endif // __IWAIT_H__
