#include "stdafx.h"

#ifdef _DEBUG
#define new DEBUG_NEW

#endif


#include "GenericClientConnection.h"

GenericClientConnection::GenericClientConnection() : hFile(INVALID_HANDLE_VALUE)
{
	memset(&olp, '\0', sizeof(&olp) );
	olp.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}

GenericClientConnection::~GenericClientConnection()
{
	CloseHandle(olp.hEvent);
	CloseHandle(hFile);
	CloseThreadpoolIo(m_pio);
}

// virtual
PTP_IO
GenericClientConnection::pio()
{
	return m_pio;
}

// virtual
void
GenericClientConnection::setPio(PTP_IO p)
{
	m_pio = p;
}

// virtual
HANDLE
GenericClientConnection::handle()
{
	return hFile;
}

// virtual
LPOVERLAPPED
GenericClientConnection::overlapped()
{
	return &olp;
}

// virtual
void
GenericClientConnection::setHandle(HANDLE h)
{
	hFile = h;
}
