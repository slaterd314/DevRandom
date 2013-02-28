#pragma once
#ifndef __GENERICWORK_H__
#define __GENERICWORK_H__

#include "IWork.h"
#include <functional>

class GenericWork : public IWork
{
	FuncPtr m_func;
public:
	GenericWork(){}
	GenericWork(const FuncPtr &func) : m_func(func)
	{
	}
	
	virtual bool Execute(PTP_CALLBACK_INSTANCE Instance)
	{
		m_func(Instance,this);
		return true;
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
