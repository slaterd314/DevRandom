#pragma once
#ifndef __GENERICWORK_H__
#define __GENERICWORK_H__
#include "IThreadPoolItemImpl.h"
#include "IWork.h"
#include <functional>

class GenericWork : public IThreadPoolItemImpl<IWork>
{
	typedef IThreadPoolItemImpl<IWork> super;
	FuncPtr m_func;
public:
	GenericWork();
	virtual void Execute(PTP_CALLBACK_INSTANCE Instance);
	void setWork(const IWork::FuncPtr &func);
	virtual PTP_WORK handle();
	virtual bool SubmitWork();
	virtual bool CloseWork();
	virtual void WaitForCallbacks(BOOL bCancelPendingCallbacks);
};


#endif // __GENERICWORK_H__
