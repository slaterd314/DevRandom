#pragma once
#ifndef __GENERICWORK_H__
#define __GENERICWORK_H__

#include <functional>

class GenericWork : public IWork
{
	::std::function<bool (PTP_CALLBACK_INSTANCE , IWork *)> m_func;
public:
	GenericWork(){}
	GenericWork(const ::std::function<bool (PTP_CALLBACK_INSTANCE , IWork *)> &func) : m_func(func)
	{
	}
	
	virtual bool Execute(PTP_CALLBACK_INSTANCE Instance)
	{
		return m_func(Instance,this);
	}
	void setFunc(const ::std::function<bool (PTP_CALLBACK_INSTANCE , IWork *)> &func)
	{
		m_func = func;
	}
};

#endif // __GENERICWORK_H__
