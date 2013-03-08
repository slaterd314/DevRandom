#pragma once
#ifndef __IWORK_H__
#define __IWORK_H__

#include "IThreadPoolItem.h"

/// Interface to a thread pook work object.
/// This interface wraps a PTP_WORK handle as well
/// The underlying object stores a std::function that it executes
/// when the work object is called back.
class 
__declspec(novtable)
IWork : public IThreadPoolItem
{
public:
	virtual ~IWork(){}
	/// typedef for the function this object uses.
	typedef ::std::function<void (PTP_CALLBACK_INSTANCE , IWork *)> FuncPtr;
	/// Execute the work. This method is called from the thread pool work item callback.
	virtual void Execute(PTP_CALLBACK_INSTANCE Instance)=0;
	/// Assign a new std::function object to be executed by Execute()
	virtual void setWork(const FuncPtr &f)=0;
	/// Access to the underlying windows PTP_WORK handle.
	virtual PTP_WORK handle()=0;
	/// Submit this object to the thread pool for a callback.
	virtual bool SubmitWork()=0;
	/// Close the underlying PTP_WORK handle. Once this method is called, this object is no longer valid for thread pool calls.
	virtual bool CloseWork()=0;
	/// Wait for any pending callbacks on this work object, optionally canceling any work objects
	/// that are queued but haven't run yet. (bCancelPendingCallbacks=TRUE cancels pending callbacks)
	/// Be careful using this one with bCancelPendingCallbacks == FALSE. In that case, this method will call one of the Wait API's
	/// so calling it from your thread pool threads can deadlock the pool. 
	virtual void WaitForCallbacks(BOOL bCancelPendingCallbacks)=0;
	typedef ::std::shared_ptr<class IWork> Ptr;
};

#endif // __IWORK_H__
