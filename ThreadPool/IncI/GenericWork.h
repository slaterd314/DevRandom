#pragma once
#ifndef __GENERICWORK_H__
#define __GENERICWORK_H__

#include "IWork.h"
#include <functional>

class GenericWork : public IWork
{
	FuncPtr m_func;
public:
	GenericWork() : m_func([](PTP_CALLBACK_INSTANCE , IWork *){})
	{
	}
	GenericWork(const FuncPtr &func) : m_func(func)
	{
	}
	
	virtual void Execute(PTP_CALLBACK_INSTANCE Instance)
	{
		m_func(Instance,this);
	}
	void setWork(const FuncPtr &func)
	{
		doSetWork(func);
	}
	void doSetWork(const FuncPtr &func)
	{
		m_func = func;
	}
};

#endif // __GENERICWORK_H__
