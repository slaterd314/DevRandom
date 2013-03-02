#pragma once
#ifndef __GENERICIOCOMPLETION_H__
#define __GENERICIOCOMPLETION_H__

#include "IIoCompletion.h"
#include <functional>

class GenericIoCompletion : public IIoCompletion
{
	FuncPtr m_func;
public:
	GenericIoCompletion() : m_func([](PTP_CALLBACK_INSTANCE , PVOID, ULONG, ULONG_PTR, IIoCompletion *){})
	{
	}
	GenericIoCompletion(const FuncPtr &func) : m_func(func)
	{
	}
	
	virtual void OnComplete(PTP_CALLBACK_INSTANCE Instance, PVOID Overlapped, ULONG IoResult, ULONG_PTR nBytesTransfered)
	{
		m_func(Instance,Overlapped,IoResult,nBytesTransfered,this);
	}
	void setIoComplete(const FuncPtr &func)
	{
		DoSetIoComplete(func);
	}
	void DoSetIoComplete(const FuncPtr &func)
	{
		m_func = func;
	}
};

#endif // __GENERICIOCOMPLETION_H__
