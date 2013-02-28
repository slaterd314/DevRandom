#include "stdafx.h"

#ifdef _DEBUG
#define new DEBUG_NEW

#endif


#include "PipeClientConnection.h"
#include "IThreadPool.h"
#include "IWork.h"

/*static*/
LPCTSTR PipeClientConnection::lpszPipename = TEXT("\\\\.\\pipe\\random");

PipeClientConnection::PipeClientConnection() : 
hPipe(INVALID_HANDLE_VALUE), 
m_state(UNCONNECTED), 
m_pWork(NULL)
{
	TRACE(TEXT(">>>PipeClientConnection::PipeClientConnection(), this=0x%x.\n"), this); 
	memset(&olp, '\0', sizeof(OVERLAPPED));
	olp.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);	
}

PipeClientConnection::~PipeClientConnection()
{
	CloseHandle(olp.hEvent);
	CloseHandle(hPipe);
	TRACE(TEXT("<<<PipeClientConnection::~PipeClientConnection(), this=0x%x.\n"), this);
}

/* virtual */
IClientConnection *
PipeClientConnection::createNew()
{
	return pool()->newNamedPipeConnection();
}

/* virtual */
HANDLE
PipeClientConnection::handle()
{
	return hPipe; 
}

/* virtual */
LPOVERLAPPED
PipeClientConnection::overlapped()
{
	return &olp; 
}
/* virtual */
bool
PipeClientConnection::ListenForNewConnection()
{
	bool bRetVal = false;
	hPipe = CreateNamedPipe(lpszPipename,
					PIPE_ACCESS_OUTBOUND|FILE_FLAG_OVERLAPPED,
					PIPE_TYPE_MESSAGE|PIPE_WAIT,
					PIPE_UNLIMITED_INSTANCES,
					BUFSIZE,
					BUFSIZE,
					0,
					NULL);
	if( INVALID_HANDLE_VALUE == hPipe )
	{
		_ftprintf_s(stderr,TEXT("CreateNamedPipe failed, GLE=%d.\n"), GetLastError()); 
	}
	
	if( pool()->CreateThreadpoolIo(this) )
	{
		pool()->StartThreadpoolIo(this); // you need to call this before every asynchronous operation

		
		BOOL fConnected = ConnectNamedPipe(handle(), overlapped());	// asynchronous ConnectNamedPipe() call - this call will return immediately
															// and the Io completion routine will be called when a client connects.
		if( !fConnected )
		{
			if( GetLastError() == ERROR_IO_PENDING || GetLastError() == ERROR_PIPE_CONNECTED )
				fConnected = TRUE;
		}
		if( fConnected )
		{
			m_state = LISTENING;
			bRetVal = true;
		}
		else
		{
			pool()->CancelThreadpoolIo(this);	// prevent a memory leak if ConnectNamedPipe() fails.
		}
	}
	
	return bRetVal;
}

// Io Completion callback
/* virtual */
bool
PipeClientConnection::OnComplete(PTP_CALLBACK_INSTANCE /*Instance*/, PVOID /*Overlapped*/, ULONG IoResult, ULONG_PTR /*nBytesTransfered*/, PTP_IO /*pio*/)
{
	if( NO_ERROR == IoResult )
	{
		switch(m_state)
		{
			case LISTENING:
				return beginIo();
				break;
			case CONNECTED:
				return writeData();
				break;

		}
		return true;
	}
	else
	{
		_ftprintf_s(stderr, TEXT("OnComplete IoResult == %d\n"), IoResult );
		return false;
	}
}

/* virtual */
bool
PipeClientConnection::beginIo()
{
	// IO completion - client has just connected
	// to the pipe. Create a WriteRandomData3 work packet
	// for the client.
	bool bRetVal = false;
	m_pWork = pool()->newWriteRandomData3(this);
	if( m_pWork )
	{
		// switch out state to CONNECTED and submit the write data work packet
		m_state = CONNECTED;
		pool()->SubmitThreadpoolWork(m_pWork);
		// This object is now being used for the client connection.
		// Create a new PipeClientConnection object and have it start listening for more connections.
		IWork *pWork = pool()->newWaitForNewConnection(this->createNew());
		if( pWork )
			pool()->SubmitThreadpoolWork(pWork);

		bRetVal = true;
	}
	return bRetVal;
}

/* virtual */
bool
PipeClientConnection::writeData()
{
	// the last WriteFile callback
	// completed - re-submit our work object
	// to initiate a new write.
	bool bRetVal = false;
	if(m_pWork)
		bRetVal = pool()->SubmitThreadpoolWork(m_pWork);
	return bRetVal;
}
