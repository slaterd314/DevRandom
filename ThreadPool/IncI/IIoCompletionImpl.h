#pragma once
#ifndef __IIOCOMPLETIONIMPL_H__
#define __IIOCOMPLETIONIMPL_H__

template <class C>
class IIoCompletionImpl : public C
{
	PTP_IO m_pio;
public:
	virtual PTP_IO pio()
	{
		return m_pio;
	}
	IIoCompletionImpl()
	{
	}
	virtual void setPio(PTP_IO p)
	{
		m_pio = p;
	}
	virtual PTP_IO handle()
	{
		return m_pio;
	}
	virtual bool StartIo()
	{
		return pool() ? pool()->StartThreadpoolIo(this) : false;
	}
	virtual bool CloseIo()
	{
		return pool() ? pool()->CloseThreadpoolIo(this) : false;
	}
	virtual bool CancelIo()
	{
		return pool() ? pool()->CancelThreadpoolIo(this) : false;
	}
	virtual void WaitForCallbacks(BOOL bCancelPendingCallbacks)
	{
		if( pool() )
			pool()->WaitForThreadpoolIoCallbacks(this, bCancelPendingCallbacks);
	}
};



#endif // __IIOCOMPLETIONIMPL_H__
