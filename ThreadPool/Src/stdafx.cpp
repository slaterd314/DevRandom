#include "stdafx.h"


#pragma hdrstop


#ifdef _DEBUG

#include <cstdarg>

void TRACE(LPCTSTR lpszFormat, ...)
{
	va_list arg;
	va_start(arg, lpszFormat);
	_vftprintf_s(stderr, lpszFormat, arg);
	va_end(arg);
}

#endif // _DEBUG
