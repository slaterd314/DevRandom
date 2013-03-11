#include "stdafx.h"
#include "GenericTimer.h"

GenericTimer::GenericTimer() : m_func([](PTP_CALLBACK_INSTANCE, ITimer *){})
{
}

void
GenericTimer::Execute(PTP_CALLBACK_INSTANCE Instance)
{
	m_func(Instance,this);
}

void
GenericTimer::setTimer(const FuncPtr &func)
{
	m_func = func;
}

PTP_TIMER
GenericTimer::handle()
{
	return reinterpret_cast<PTP_TIMER>(super::getHandle());
}

bool
GenericTimer::SetThreadpoolTimer(PFILETIME pftDueTime, DWORD msPeriod, DWORD msWindowLength)
{
	return pool() ? pool()->SetThreadpoolTimer(pftDueTime,msPeriod,msWindowLength,this) : false;
}

bool
GenericTimer::CloseTimer()
{
	return pool() ? pool()->CloseThreadpoolTimer(this) : false;
}

void
GenericTimer::WaitForCallbacks(BOOL bCancelPendingCallbacks)
{
	if( pool() )
		pool()->WaitForThreadpoolTimerCallbacks(this, bCancelPendingCallbacks);
}
