#pragma once
#ifndef __PIPECLIENTCONNECTION_H__
#define __PIPECLIENTCONNECTION_H__

#ifdef DEPRECATED

#include "IClientConnection.h"
#include "IWork.h"

class PipeClientConnection : public IClientConnection // , public IIoCompletion
{
	enum { BUFSIZE=256 };
	enum states {
		UNCONNECTED,
		LISTENING,
		CONNECTED,
		DISCONNECTED
	};
public:
	virtual IClientConnection *createNew();
	PipeClientConnection();
	~PipeClientConnection();
	virtual HANDLE handle();
	virtual LPOVERLAPPED overlapped();
	virtual bool ListenForNewConnection();
	// Io Completion callback
	virtual bool OnComplete(PTP_CALLBACK_INSTANCE /*Instance*/, PVOID /*Overlapped*/, ULONG IoResult, ULONG_PTR /*nBytesTransfered*/, PTP_IO /*pio*/);
	virtual bool beginIo();
	virtual bool writeData();
	IWorkPtr createPipePacket();
private:
	static LPCTSTR lpszPipename;
	BYTE   m_buffer[BUFSIZE];
	OVERLAPPED olp;
	HANDLE hPipe;	
	states m_state;
	class IWork *m_pWork;
};

#endif // DEPRECATED

#endif // __PIPECLIENTCONNECTION_H__
