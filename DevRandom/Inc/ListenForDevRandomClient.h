#pragma once
#ifndef __LISTENFORRANDOMCLIENT_H__
#define __LISTENFORRANDOMCLIENT_H__

#include "MyOverlapped.h"
#include "IIoCompletion.h"
#include "IThreadPool.h"
#include "IWork.h"
#include "SpinLock.h"
#include "RandomDataPipeServer.h"


class ListenForDevRandomClient : public ::std::enable_shared_from_this<ListenForDevRandomClient>, public IDevRandomServer
{
public:
	typedef ::std::shared_ptr<class ListenForDevRandomClient> Ptr;
private:
	IWorkPtr			m_work;
	IIoCompletionPtr	m_pio;
	HANDLE				m_hPipe;
	MyOverlappedPtr		m_olp;
	::std::_tstring		m_pipeName;
	Ptr					m_self;
	HANDLE				m_hStopEvent;
	bool				m_stop;
public:
	typedef ::std::shared_ptr<class ListenForDevRandomClient> Ptr;
	ListenForDevRandomClient(LPCTSTR lpszPipeName, HANDLE hStopEvent, IThreadPool *pPool);
	~ListenForDevRandomClient();
	virtual bool runServer();
	void makeSelfReference();
	static Ptr create(LPCTSTR lpszPipeName, HANDLE hStopEvent, IThreadPool *pPool);
	bool startServer();
	void listenForClient(PTP_CALLBACK_INSTANCE , IWork *pWork);
	void onConnectClient(PTP_CALLBACK_INSTANCE , PVOID Overlapped, ULONG IoResult, ULONG_PTR, IIoCompletion *pIo);
	virtual void shutDownServer();
};

#endif // __LISTENFORRANDOMCLIENT_H__
