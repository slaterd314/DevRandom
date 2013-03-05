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
};



#endif // __IIOCOMPLETIONIMPL_H__
