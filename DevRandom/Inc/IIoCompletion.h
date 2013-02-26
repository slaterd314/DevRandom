#pragma once
#ifndef __IIOCOMPLETION_H__
#define __IIOCOMPLETION_H__
/*

#include "IThreadPoolItem.h"
class
__declspec(novtable)
IIoCompletion : virtual public IThreadPoolItem
{
public:
	virtual ~IIoCompletion() {}
	virtual bool OnComplete(PTP_CALLBACK_INSTANCE Instance, PVOID Overlapped, ULONG IoResult, ULONG_PTR nBytesTransfered, PTP_IO pio)=0;
	static VOID CALLBACK callback(	__inout      PTP_CALLBACK_INSTANCE Instance,
								  __inout_opt  PVOID Context,
								  __inout_opt  PVOID Overlapped,
								  __in         ULONG IoResult,
								  __in         ULONG_PTR NumberOfBytesTransferred,
								  __inout      PTP_IO Io);
};

*/

#endif // __IIOCOMPLETION_H__
