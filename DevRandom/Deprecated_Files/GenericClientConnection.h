#pragma once
#ifndef __GENERICCLIENTCONNECTION_H__
#define __GENERICCLIENTCONNECTION_H__

#include "IGenericClientConnection.h"

class GenericClientConnection : public IGenericClientConnection
{
	HANDLE hFile;
	OVERLAPPED olp;
	PTP_IO m_pio;
	enum { BUFSIZE=256 };
	enum states {
		UNCONNECTED,
		LISTENING,
		CONNECTED,
		DISCONNECTED
	};
public:
	GenericClientConnection();
	~GenericClientConnection();
	virtual PTP_IO	pio();
	virtual void setPio(PTP_IO p);
	virtual HANDLE handle();
	virtual LPOVERLAPPED overlapped();
	virtual void setHandle(HANDLE h);
};

#endif// __GENERICCLIENTCONNECTION_H__
