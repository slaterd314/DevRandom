#pragma once
#ifndef __LISTENFORRANDOMCLIENT_H__
#define __LISTENFORRANDOMCLIENT_H__

#include "MyOverlapped.h"
#include "IIoCompletion.h"
#include "IThreadPool.h"
#include "IWork.h"
#include "IWait.h"
#include "SpinLock.h"
#include "RandomDataPipeServer.h"

/// Named pipe server class. THis object listens for incomming connections to the named pipe.
class ListenForDevRandomClient : public ::std::enable_shared_from_this<ListenForDevRandomClient>, public IDevRandomServer
{
public:
	typedef ::std::shared_ptr<class ListenForDevRandomClient> Ptr;
private:
	IWork::Ptr			m_work;
	IWait::Ptr			m_wait;
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
	void makeSelfReferent();
	static Ptr create(LPCTSTR lpszPipeName, HANDLE hStopEvent, IThreadPool *pPool);
	bool startServer();
	// Work item that sets up an asynchronous ConnectNamedPipe
	void listenForClient(PTP_CALLBACK_INSTANCE , IWork *pWork);
	// Wait item that gets called when a client connects to the named pipe and the event member of
	// the overlapped structure is signaled.
	void onConnectEventSignaled(PTP_CALLBACK_INSTANCE , TP_WAIT_RESULT , IWait *);
	virtual void CheckHandle();
	virtual void shutDownServer();
};

#endif // __LISTENFORRANDOMCLIENT_H__
