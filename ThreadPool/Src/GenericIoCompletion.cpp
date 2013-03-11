#include "stdafx.h"
#include "GenericIoCompletion.h"


GenericIoCompletion::GenericIoCompletion() : m_func([](PTP_CALLBACK_INSTANCE , PVOID, ULONG, ULONG_PTR, IIoCompletion *){})
{
}
	
void GenericIoCompletion::OnComplete(PTP_CALLBACK_INSTANCE Instance, PVOID Overlapped, ULONG IoResult, ULONG_PTR nBytesTransfered)
{
	m_func(Instance,Overlapped,IoResult,nBytesTransfered,this);
}
void GenericIoCompletion::setIoComplete(const FuncPtr &func)
{
	m_func = func;
}

void GenericIoCompletion::setPio(PTP_IO p)
{
	super::setHandle(p);
}
PTP_IO GenericIoCompletion::handle()
{
	return reinterpret_cast<PTP_IO>(super::getHandle());
}
bool GenericIoCompletion::StartIo()
{
	return pool() ? pool()->StartThreadpoolIo(this) : false;
}
bool GenericIoCompletion::CloseIo()
{
	return pool() ? pool()->CloseThreadpoolIo(this) : false;
}
bool GenericIoCompletion::CancelIo()
{
	return pool() ? pool()->CancelThreadpoolIo(this) : false;
}
void GenericIoCompletion::WaitForCallbacks(BOOL bCancelPendingCallbacks)
{
	if( pool() )
		pool()->WaitForThreadpoolIoCallbacks(this, bCancelPendingCallbacks);
}


