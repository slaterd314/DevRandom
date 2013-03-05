#include "stdafx.h"


#pragma hdrstop


#ifdef _DEBUG

#include <cstdarg>

#ifdef TRACE
#undef TRACE
#endif


void TRACE(LPCTSTR lpszFormat, ...)
{
	va_list arg;
	va_start(arg, lpszFormat);
	_vftprintf_s(stderr, lpszFormat, arg);
	va_end(arg);
}

#endif // _DEBUG
