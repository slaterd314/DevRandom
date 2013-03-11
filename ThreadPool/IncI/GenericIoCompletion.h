#pragma once
#ifndef __GENERICIOCOMPLETION_H__
#define __GENERICIOCOMPLETION_H__

#include "IIoCompletion.h"
#include <functional>
#include "IThreadPoolItemImpl.h"

class GenericIoCompletion : public IThreadPoolItemImpl<IIoCompletion>
{
	typedef IThreadPoolItemImpl<IIoCompletion> super;
	FuncPtr m_func;
public:
	GenericIoCompletion();
	virtual void OnComplete(PTP_CALLBACK_INSTANCE Instance, PVOID Overlapped, ULONG IoResult, ULONG_PTR nBytesTransfered);
	void setIoComplete(const FuncPtr &func);
	virtual void setPio(PTP_IO p);
	virtual PTP_IO handle();
	virtual bool StartIo();
	virtual bool CloseIo();
	virtual bool CancelIo();
	virtual void WaitForCallbacks(BOOL bCancelPendingCallbacks);
};

#endif // __GENERICIOCOMPLETION_H__
