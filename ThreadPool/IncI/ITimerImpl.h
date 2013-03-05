#pragma once
#ifndef __ITIMERIMPL_H__
#define __ITIMERIMPL_H__

template <class C>
class ITimerImpl : public C
{
	PTP_TIMER m_ptpTimer;
public:
	ITimerImpl()
	{
		//TRACE(TEXT(">>>IWorkImpl::IWorkImpl<%S>(), this=0x%x.\n"), typeid(C).name() , this); 
	}
	~ITimerImpl()
	{
		// TRACE(TEXT("IWorkImpl::~IWorkImpl<%S>(), this=0x%x.\n"), typeid(C).name() , this); 
	}
	virtual PTP_TIMER handle() { return m_ptpTimer; }
	virtual void setPtp(PTP_TIMER t) { m_ptpTimer = t; }
};


#endif // __ITIMERIMPL_H__
