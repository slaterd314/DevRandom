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
	virtual bool SetThreadpoolTimer(PFILETIME pftDueTime, DWORD msPeriod, DWORD msWindowLength)
	{
		return pool() ? pool()->SetThreadpoolTimer(pftDueTime,msPeriod,msWindowLength,this) : false;
	}
	virtual bool CloseTimer()
	{
		return pool() ? pool()->CloseThreadpoolTimer(this) : false;
	}

	virtual void WaitForCallbacks(BOOL bCancelPendingCallbacks)
	{
		if( pool() )
			pool()->WaitForThreadpoolTimerCallbacks(this, bCancelPendingCallbacks);
	}
};


#endif // __ITIMERIMPL_H__
