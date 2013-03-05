#include "stdafx.h"


#pragma hdrstop


#ifdef _DEBUG

#include <cstdarg>

#ifdef TRACE
#undef TRACE
#endif

void TRACE(LPCTSTR lpszFormat, ...)
{
	TCHAR buffer[2048];
	va_list arg;
	va_start(arg, lpszFormat);
	_vsntprintf_s(buffer,_TRUNCATE, lpszFormat, arg);
	va_end(arg);
	::OutputDebugString(buffer);
}

#endif // _DEBUG