#pragma once
#ifndef __GENERICWAIT_H__
#define __GENERICWAIT_H__

#include "IWait.h"
#include <functional>

class GenericWait : public IWait
{
	FuncPtr m_func;
public:
	GenericWait() : m_func([](PTP_CALLBACK_INSTANCE , TP_WAIT_RESULT , IWait *){})
	{
	}
	GenericWait(const FuncPtr &func) : m_func(func)
	{
	}
	
	virtual void Execute(PTP_CALLBACK_INSTANCE Instance, TP_WAIT_RESULT WaitResult)
	{
		m_func(Instance,WaitResult, this);
	}
	void setWait(const FuncPtr &func)
	{
		m_func = func;
	}
};

#endif // __GENERICWAIT_H__
