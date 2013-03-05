#pragma once
#ifndef __IWORKIMPL_H__
#define __IWORKIMPL_H__


class IWorkImplPrivate
{
public:
	virtual void setPtp(PTP_WORK w)=0;
};

template <class C>
class IWorkImpl : public C, public IWorkImplPrivate
{
	PTP_WORK m_ptpWork;
public:
	IWorkImpl()
	{
		//TRACE(TEXT(">>>IWorkImpl::IWorkImpl<%S>(), this=0x%x.\n"), typeid(C).name() , this); 
	}
	~IWorkImpl()
	{
		// TRACE(TEXT("IWorkImpl::~IWorkImpl<%S>(), this=0x%x.\n"), typeid(C).name() , this); 
	}
	virtual void setWork(const IWork::FuncPtr &f)
	{
		__if_exists(C::doSetWork) {
			C::doSetWork(f);
		}
		__if_not_exists(C::doSetWork) {
			UNREFERENCED_PARAMETER(f);
		}
	}
	virtual PTP_WORK handle() { return m_ptpWork; }
	virtual void setPtp(PTP_WORK w) { m_ptpWork = w; }
};


#endif // __IWORKIMPL_H__
