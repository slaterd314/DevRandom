#pragma once
#ifndef __IIOCOMPLETION_H__
#define __IIOCOMPLETION_H__

#include "IThreadPoolItem.h"
#include <functional>
#include <memory>

class
__declspec(novtable)
IIoCompletion : public IThreadPoolItem
{
public:
	virtual ~IIoCompletion() {}
	/// typedef for the function this object uses.
	typedef ::std::function<void (PTP_CALLBACK_INSTANCE , PVOID, ULONG, ULONG_PTR, IIoCompletion *)> FuncPtr;
	/// Execute the completion routine. This method is called from the thread pool IoCompletion item callback.
	virtual void OnComplete(PTP_CALLBACK_INSTANCE Instance, PVOID Overlapped, ULONG IoResult, ULONG_PTR nBytesTransfered)=0;
	/// Assign a new std::function object to be executed by OnComplete()
	virtual void setIoComplete(const FuncPtr &f)=0;
	/// Access to the underlying windows PTP_TIMER handle.
	virtual PTP_IO handle()=0;
	/// submit this IoCompletion object to the thread pool. Every time you call an asynchronous IO
	/// function, you need to call this first. 
	virtual bool StartIo()=0;
	/// Close the underlying PTP_IO handle. Once this method is called, this object is no longer valid for thread pool calls.
	virtual bool CloseIo()=0;
	/// Cancel the notification submitted by StartIo(). If your asynchronous I.O. call returns an error, you need to call this
	/// to avoid a memory leak.
	virtual bool CancelIo()=0;
	/// Wait for any pending callbacks on this IoCompletion object, optionally canceling any queued 
	/// I.O. operations that haven't started yet. (bCancelPendingCallbacks=TRUE cancels pending callbacks)
	/// Be careful using this one with bCancelPendingCallbacks == FALSE. In that case, this method will call one of the Wait API's
	/// so calling it from your thread pool threads can deadlock the pool. 
	/// Also, if bCancelPendingCallbacks is TRUE, then this method will cancel any queued completion routines
	/// that haven't started yet. However, it will not wait for any Io Completion routines that are in progress. You need to
	/// call GetOverlappedResults() to determine if all pending Io operations have finished and it is safe to delete your
	/// OVERLAPPED object. see Microsoft doc for CancelThreadpoolIo() for more information. 
	virtual void WaitForCallbacks(BOOL bCancelPendingCallbacks)=0;
	typedef ::std::shared_ptr<class IIoCompletion> Ptr;
};

#endif // __IIOCOMPLETION_H__
