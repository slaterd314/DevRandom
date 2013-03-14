// stdafx.cpp : source file that includes just the standard includes
// QuickSort.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

// TODO: reference any additional headers you need in STDAFX.H
// and not in this file


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
#endif
