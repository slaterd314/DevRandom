#pragma once
#ifndef __STDAFX_H__
#define __STDAFX_H__

#define _WIN32_WINNT _WIN32_WINNT_WIN7
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <crtdbg.h>
#include <new>



#define SystemFunction036 NTAPI SystemFunction036
#include <NtSecAPI.h>
#undef SystemFunction036

#pragma warning( disable : 4995 )
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <Strsafe.h>
#include <intrin.h>
#pragma warning( default : 4995 )

#ifdef _DEBUG

void TRACE(LPCTSTR lpszFormat, ...);

#define THIS_FILE __FILE__
#define DEBUG_NEW new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#define DEBUG_DELETE delete

#else

#define TRACE __noop

#endif

#endif // __STDAFX_H__