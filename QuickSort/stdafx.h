// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define _WIN32_WINNT _WIN32_WINNT_WIN7
#define WIN32_LEAN_AND_MEAN

#include "targetver.h"

#define _CRT_RAND_S
#include <stdlib.h>

#include <windows.h>

#include <crtdbg.h>
#include <new>

#ifdef _DEBUG
#define new DEBUG_NEW

#endif


#include <stdio.h>
#include <tchar.h>


#pragma warning( disable : 4995 )
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <Strsafe.h>
#include <intrin.h>
#include <memory>
#include <functional>

#include <string>

#if defined(_UNICODE) || defined(UNICODE)
#define _tstring wstring
#else
#define _tstring string
#endif

#pragma warning( default : 4995 )

#ifdef _DEBUG

void TRACE(LPCTSTR lpszFormat, ...);

#define THIS_FILE __FILE__
#define DEBUG_NEW new(_NORMAL_BLOCK, THIS_FILE, __LINE__)

#else

#define TRACE __noop

#endif

// TODO: reference additional headers your program requires here
