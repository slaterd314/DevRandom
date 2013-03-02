#pragma once
#ifndef __SERVICEMANAGER_H__
#define __SERVICEMANAGER_H__

#include "IThreadPool.h"
#include "RandomDataPipeServer.h"

class ServiceManager
{
	enum {
		SVC_ERROR=1
	};
public:

	static VOID SvcInstall(void);
	static VOID SvcRemove();
	static VOID RunService();

private:
	static VOID WINAPI SvcMain( DWORD, LPTSTR * ); 
  
	static VOID ReportSvcStatus( DWORD, DWORD, DWORD );
	static VOID SvcInit( DWORD, LPTSTR * ); 
	static VOID SvcReportEvent( LPTSTR );

	static DWORD WINAPI HandlerEx(__in  DWORD dwControl, __in  DWORD dwEventType, __in  LPVOID lpEventData, __in  LPVOID lpContext );

private:
	static SERVICE_STATUS          gSvcStatus; 
	static SERVICE_STATUS_HANDLE   gSvcStatusHandle; 
	//static HANDLE                  ghSvcStopEvent;
	static IThreadPoolPtr			pPool;
	static LPCTSTR			lpszServiceName;
	static LPCTSTR			lpszServiceDescription;
	static IDevRandomServer::Ptr m_Server;
	static HANDLE			m_hEvent;

};


#endif // __SERVICEMANAGER_H__
