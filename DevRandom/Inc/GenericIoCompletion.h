#pragma once
#ifndef __GENERICIOCOMPLETION_H__
#define __GENERICIOCOMPLETION_H__

#include "IIoCompletion.h"
#include <functional>

class GenericIoCompletion : public IIoCompletion
{
	FuncPtr m_func;
public:
	GenericIoCompletion(){}
	GenericIoCompletion(const FuncPtr &func) : m_func(func)
	{
	}
	
	virtual bool OnComplete(PTP_CALLBACK_INSTANCE Instance, PVOID Overlapped, ULONG IoResult, ULONG_PTR nBytesTransfered)
	{
		return m_func(Instance,Overlapped,IoResult,nBytesTransfered,this);
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
