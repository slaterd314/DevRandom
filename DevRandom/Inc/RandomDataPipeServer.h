#ifndef __RANDOMDATAPIPESERVER_H__
#define __RANDOMDATAPIPESERVER_H__

#include "IIoCompletion.h"
#include "IThreadPool.h"
#include "IWork.h"

struct IDevRandomServer
{
	typedef ::std::shared_ptr<IDevRandomServer> Ptr;
	virtual ~IDevRandomServer(){}
	virtual bool runServer()=0;
};	

// IWorkPtr makePipeServer(LPCTSTR lpszPipeName, IThreadPool *pPool);
IDevRandomServer::Ptr createPipeServer(LPCTSTR lpszPipeName, HANDLE hStopEvent, IThreadPool *pPool);



#endif //__RANDOMDATAPIPESERVER_H__
