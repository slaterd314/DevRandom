#include "stdafx.h"
#include "IIoCompletion.h"
#include "IThreadPool.h"
#include "IWork.h"

#ifdef _DEBUG
#define new DEBUG_NEW

#endif


struct MyOverlapped : public OVERLAPPED
{
	enum { BUFSIZE=256 };
	unsigned __int8 *buffer;
	unsigned __int8 buffer1[BUFSIZE];
	unsigned __int8 buffer2[BUFSIZE];
	MyOverlapped()
	{
		buffer = buffer1;
	}
};
	

auto noio = [=](PTP_CALLBACK_INSTANCE , PVOID, ULONG, ULONG_PTR, IIoCompletion * /* pIo */ ) {};

IWorkPtr makePipeServer(LPCTSTR lpszPipeName, IThreadPool *pPool)
{
	//IWorkPtr pWrk;
	auto connect = [=](PTP_CALLBACK_INSTANCE , IWork *pWork) {
		if( pWork )
		{
			IWorkPtr pItem(::std::dynamic_pointer_cast<IWork>(pWork->shared_from_this()));
			
			::std::shared_ptr<MyOverlapped> olp(new MyOverlapped);
			memset(olp.get(),'\0',sizeof(MyOverlapped));

			HANDLE hPipe = CreateNamedPipe(lpszPipeName,
							PIPE_ACCESS_OUTBOUND|FILE_FLAG_OVERLAPPED,
							PIPE_TYPE_MESSAGE|PIPE_WAIT,
							PIPE_UNLIMITED_INSTANCES,
							MyOverlapped::BUFSIZE,
							MyOverlapped::BUFSIZE,
							0,
							NULL);
			if( INVALID_HANDLE_VALUE == hPipe )
			{
				_ftprintf_s(stderr,TEXT("CreateNamedPipe failed, GLE=%d.\n"), GetLastError()); 
			}
		
			// initialize it with a donothing function. 
			IIoCompletionPtr pio = pPool->newIoCompletion(hPipe, noio);

			auto beginIo = [=](PTP_CALLBACK_INSTANCE , PVOID, ULONG IoResult, ULONG_PTR, IIoCompletion *pIo) {
				IWorkPtr pServerItem(pItem);
				if( pIo && (NO_ERROR == IoResult) )
				{
					IIoCompletionPtr pio2 = pio;
					HANDLE h = hPipe;

					::std::shared_ptr<MyOverlapped> olp2(olp);

					olp->buffer = olp->buffer1;
					RtlGenRandom(olp->buffer, MyOverlapped::BUFSIZE);
			
					auto write = [=](PTP_CALLBACK_INSTANCE , IWork * /*pWork*/) {

						DWORD cbReplyBytes = MyOverlapped::BUFSIZE;
						DWORD cbWritten = 0;

						unsigned __int8 *pWritePtr = olp2->buffer;
						if( olp2->buffer == olp2->buffer1 )
							olp2->buffer = olp2->buffer2;
						else
							olp2->buffer = olp2->buffer1;
						unsigned __int8 *pGeneratePtr = olp2->buffer;

						if( pio2.get() )
							pio2->pool()->StartThreadpoolIo(pio2.get());
						// Write the reply to the pipe. 
						BOOL fSuccess = WriteFile( 
									h,        // handle to pipe 
									pWritePtr,     // buffer to write from 
									cbReplyBytes, // number of bytes to write 
									&cbWritten,   // number of bytes written 
									olp2.get());        // overlapped I/O 

						if(!fSuccess)
						{
							if( ERROR_IO_PENDING == GetLastError() )
								fSuccess = TRUE;
						}
						if (!fSuccess /*|| cbReplyBytes != cbWritten*/ )
						{   
							_ftprintf_s(stderr,TEXT("makePipeServer::write failed, GLE=%d.\n"), GetLastError());
						}
						else if( FALSE == RtlGenRandom(pGeneratePtr, MyOverlapped::BUFSIZE) )
						{
							pio2->pool()->CancelThreadpoolIo(pio2.get());
							const_cast<IIoCompletionPtr &>(pio2).reset();	// release our hold on the IoCompletion object
						}
					};

					IWorkPtr pItem2 = pIo->pool()->newWork(write);
						
					if( pItem2.get() )
					{
						//IIoCompletionPtr pio3 = pio;
						auto kickWrite = [=](PTP_CALLBACK_INSTANCE , PVOID, ULONG IoResult, ULONG_PTR, IIoCompletion *pIo) {
							if( NO_ERROR == IoResult )
							{
								if( pItem2.get() )
								{
									pIo->pool()->SubmitThreadpoolWork(pItem2.get());
								}
							}
							else
								const_cast<IWorkPtr &>(pItem2).reset();	// release our hold on the work item
						};

						pio->setIoComplete(kickWrite);
						pItem2->pool()->SubmitThreadpoolWork(pItem2.get());
						// start a new listening session
						pServerItem->pool()->SubmitThreadpoolWork(pServerItem.get());

					}
					else
					{
						// release our hold on these pointers 
						// we're exiting
						const_cast<IWorkPtr &>(pItem).reset();
						const_cast<IIoCompletionPtr &>(pio).reset();
					}
				}
			};


			pio->setIoComplete(beginIo);
			pPool->StartThreadpoolIo(pio.get());
			
			//IIoCompletionPtr pio = pWork->pool()->newIoCompletion();

			BOOL fConnected = ConnectNamedPipe(hPipe, olp.get());	// asynchronous ConnectNamedPipe() call - this call will return immediately
																// and the Io completion routine will be called when a client connects.
			if( !fConnected )
			{
				if( GetLastError() == ERROR_IO_PENDING || GetLastError() == ERROR_PIPE_CONNECTED )
					fConnected = TRUE;
			}
			if( !fConnected )
			{
				pPool->CancelThreadpoolIo(pio.get());	// prevent a memory leak if ConnectNamedPipe() fails.
			}
		}
	};

	return pPool->newWork(connect);

}