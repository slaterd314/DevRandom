#pragma once
#ifndef __IIOCOMPLETION_H__
#define __IIOCOMPLETION_H__

#include "IThreadPoolItem.h"
#include <functional>
#include <memory>

class
__declspec(novtable)
IIoCompletion : virtual public IThreadPoolItem
{
public:
	virtual ~IIoCompletion() {}
	typedef ::std::function<void (PTP_CALLBACK_INSTANCE , PVOID, ULONG, ULONG_PTR, IIoCompletion *)> FuncPtr;
	virtual void OnComplete(PTP_CALLBACK_INSTANCE Instance, PVOID Overlapped, ULONG IoResult, ULONG_PTR nBytesTransfered)=0;
	virtual PTP_IO pio()=0;
	virtual void setIoComplete(const FuncPtr &f)=0;
	static VOID CALLBACK callback(	__inout      PTP_CALLBACK_INSTANCE Instance,
								  __inout_opt  PVOID Context,
								  __inout_opt  PVOID Overlapped,
								  __in         ULONG IoResult,
								  __in         ULONG_PTR NumberOfBytesTransferred,
								  __inout      PTP_IO Io);
};

typedef ::std::shared_ptr<IIoCompletion> IIoCompletionPtr;

#endif // __IIOCOMPLETION_H__
