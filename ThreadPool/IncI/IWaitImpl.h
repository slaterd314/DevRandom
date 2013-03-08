#pragma once
#ifndef __IWAITIMPL_H__
#define __IWAITIMPL_H__


template <class C>
class IWaitImpl : public C
{
	PTP_WAIT m_ptpWait;
public:
	IWaitImpl()
	{
		//TRACE(TEXT(">>>IWorkImpl::IWorkImpl<%S>(), this=0x%x.\n"), typeid(C).name() , this); 
	}
	~IWaitImpl()
	{
		// TRACE(TEXT("IWorkImpl::~IWorkImpl<%S>(), this=0x%x.\n"), typeid(C).name() , this); 
	}
	virtual PTP_WAIT handle() { return m_ptpWait; }
	virtual void setPtp(PTP_WAIT w) { m_ptpWait = w; }

	virtual bool SetThreadpoolWait(HANDLE h, PFILETIME pftTimeout)
	{
		return pool() ? pool()->SetThreadpoolWait(h,pftTimeout,this) : false;
	}

	virtual bool CloseWait()
	{
		return pool() ? pool()->CloseThreadpoolWait(this) : false;
	}
	virtual void WaitForCallbacks(BOOL bCancelPendingCallbacks)
	{
		if( pool() )
			pool()->WaitForThreadpoolWaitCallbacks(this, bCancelPendingCallbacks);
	}
};


#endif // __IWAITIMPL_H__
