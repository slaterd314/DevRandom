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
};


#endif // __IWAITIMPL_H__
