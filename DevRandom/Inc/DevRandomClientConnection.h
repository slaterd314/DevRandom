#pragma once
#ifndef __DEVRANDOMCLIENTCONNECTION_H__
#define __DEVRANDOMCLIENTCONNECTION_H__

#include "MyOverlapped.h"
#include "IIoCompletion.h"
#include "IThreadPool.h"
#include "IWork.h"
#include "SpinLock.h"

#pragma warning(disable : 4324)

class DevRandomClientConnection : public ::std::enable_shared_from_this<DevRandomClientConnection>
{
	enum StopCaller{
		WORK_CALL,
		WAIT_CALL,
		IO_CALL
	} ;
	struct lock
	{
		SpinLock &m_lock;
		lock(SpinLock &l) : m_lock(l) { m_lock.acquire(); }
		~lock() { m_lock.release(); }
	private:
		lock &operator=(const lock &) { return *this; }
	};
private:
	// copy & assignment not allowed.
	DevRandomClientConnection(const DevRandomClientConnection &other);
	DevRandomClientConnection &operator=(const DevRandomClientConnection &other);
public:
	typedef ::std::shared_ptr<class DevRandomClientConnection> Ptr;

	DevRandomClientConnection(const IIoCompletionPtr &pio, HANDLE hPipe, const MyOverlappedPtr &olp, HANDLE hStopEvent, IThreadPool *pPool);
	~DevRandomClientConnection();
	void makeSelfReference();

	static void waitForClientsToStop();

public:
	static Ptr create(const IIoCompletionPtr &pio, HANDLE hPipe, const MyOverlappedPtr &olp, HANDLE hStopEvent, IThreadPool *pPool);
	bool checkWriteFileError();
	void writeToClient(PTP_CALLBACK_INSTANCE /*Instance*/, IWork * /*pWork*/);
	void onWriteClientComplete(PTP_CALLBACK_INSTANCE , PVOID /*Overlapped*/, ULONG IoResult, ULONG_PTR, IIoCompletion *pIo);
	bool runClient();
	void onWaitSignaled(PTP_CALLBACK_INSTANCE , TP_WAIT_RESULT /*WaitResult*/, IWait * /*pWait*/);
	void doStop(PTP_CALLBACK_INSTANCE Instance, IWork * pWork);
	void doStop2(PTP_CALLBACK_INSTANCE Instance, IThreadPool * pPool);
	void Stop(StopCaller /*caller*/);
private:
	bool WriteData(const unsigned __int8 *pData, const DWORD &dwDataLength);
	void closeHandle();
private:
	IWorkPtr							m_work;
	IWorkPtr							m_workStop;
	IWaitPtr							m_wait;
	IIoCompletionPtr					m_pio;
	MyOverlappedPtr						m_olp;
	Ptr									m_self;
	LWSpinLock							m_lock;
	HANDLE								m_hPipe;
	ALIGN MACHINE_INT					m_stop;
	ALIGN MACHINE_INT					m_nOutStandingIoOps;	
	bool								m_asyncIo;
	bool								m_asyncWork;
	
	static volatile ALIGN MACHINE_INT	m_nActiveClients;
	SRWLOCK								m_SRWLock;
};

#pragma warning(default : 4324)

#endif // __DEVRANDOMCLIENTCONNECTION_H__
