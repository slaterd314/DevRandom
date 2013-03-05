#pragma once
#ifndef __GENERICTIMER_H__
#define __GENERICTIMER_H__

#include "ITimer.h"

class GenericTimer : public ITimer
{
	FuncPtr m_func;
public:
	GenericTimer() : m_func([](PTP_CALLBACK_INSTANCE, ITimer *){})
	{
	}
	GenericTimer(const FuncPtr &func) : m_func(func)
	{
	}
	
	virtual void Execute(PTP_CALLBACK_INSTANCE Instance)
	{
		m_func(Instance,this);
	}
	void setTimer(const FuncPtr &func)
	{
		m_func = func;
	}
};

#endif // __GENERICTIMER_H__
