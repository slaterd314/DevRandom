#include "stdafx.h"
#include "GenericWork.h"


GenericWork::GenericWork() :
m_func([](PTP_CALLBACK_INSTANCE , IWork *){})
{
}
	
void GenericWork::Execute(PTP_CALLBACK_INSTANCE Instance)
{
	m_func(Instance,this);
}
void GenericWork::setWork(const IWork::FuncPtr &func)
{
	m_func = func;
}
PTP_WORK
GenericWork::handle()
{
	return reinterpret_cast<PTP_WORK>(super::getHandle());
}

bool
GenericWork::SubmitWork()
{
	return pool() ? pool()->SubmitThreadpoolWork(this) : false;
}

bool GenericWork::CloseWork()
{
	return pool() ? pool()->CloseThreadpoolWork(this) : false;
}

void GenericWork::WaitForCallbacks(BOOL bCancelPendingCallbacks)
{
	if( pool() )
		pool()->WaitForThreadpoolWorkCallbacks(this, bCancelPendingCallbacks);
}
