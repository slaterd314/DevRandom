#include "stdafx.h"
#include "GenericWait.h"



GenericWait::GenericWait() : m_func([](PTP_CALLBACK_INSTANCE , TP_WAIT_RESULT , IWait *){})
{
}

void
GenericWait::Execute(PTP_CALLBACK_INSTANCE Instance, TP_WAIT_RESULT WaitResult)
{
	m_func(Instance,WaitResult, this);
}

void
GenericWait::setWait(const FuncPtr &func)
{
	m_func = func;
}

PTP_WAIT
GenericWait::handle()
{
	return reinterpret_cast<PTP_WAIT>(super::getHandle());
}

bool
GenericWait::SetThreadpoolWait(HANDLE h, PFILETIME pftTimeout)
{
	return pool() ? pool()->SetThreadpoolWait(h,pftTimeout,this) : false;
}

bool
GenericWait::CloseWait()
{
	return pool() ? pool()->CloseThreadpoolWait(this) : false;
}

void
GenericWait::WaitForCallbacks(BOOL bCancelPendingCallbacks)
{
	if( pool() )
		pool()->WaitForThreadpoolWaitCallbacks(this, bCancelPendingCallbacks);
}
