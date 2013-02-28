#include "stdafx.h"
#include "IIoCompletion.h"
#include "IThreadPool.h"
#include "IWork.h"
	
IWorkPtr makePipeServer(LPCTSTR lpszPipeName, IThreadPool *pPool);
