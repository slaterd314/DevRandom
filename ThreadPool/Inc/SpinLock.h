#pragma once
#ifndef __SPINWAIT_H__
#define __SPINWAIT_H__

#ifdef _M_X64
#define interlockedCompareExchange(a,b,c) _InterlockedCompareExchange64(a,b,c)
#define interlockedIncrement(a) InterlockedIncrement64(a)
#define interlockedDecrement(a) InterlockedDecrement64(a)
typedef __int64 MACHINE_INT;
#define ALIGN __declspec(align(64))
#else
#define interlockedCompareExchange(a,b,c) InterlockedCompareExchange(a,b,c)
#define interlockedIncrement(a) InterlockedIncrement(a)
#define interlockedDecrement(a) InterlockedDecrement(a)
typedef unsigned __int32 MACHINE_INT;
#define ALIGN __declspec(align(32))
#endif

class
ALIGN
SpinLock
{
	volatile ALIGN MACHINE_INT m_n;
	__forceinline
	void ChangeIf(int t, int n)
	{
		ALIGN MACHINE_INT test = t;
		ALIGN MACHINE_INT newVal = n;
		while( interlockedCompareExchange(&m_n, newVal, test) != test )
			;
	}
public:
	SpinLock(bool bInitialLock) : m_n(bInitialLock ? 1 : 0 ){}
	__forceinline void acquire()
	{
		ChangeIf(0,1);
	}
	__forceinline void release()
	{
		ChangeIf(1,0);
	}
};

class
ALIGN
SpinWait
{
	volatile ALIGN MACHINE_INT m_n;
	__forceinline
	void ChangeIf(int t, int n)
	{
		ALIGN MACHINE_INT test = t;
		ALIGN MACHINE_INT newVal = n;
		while( interlockedCompareExchange(&m_n, newVal, test) != test )
			;
	}
public:
	SpinWait(bool bInitialLock) : m_n(bInitialLock ? 1 : 0 ){}
	__forceinline void inc()
	{
		interlockedIncrement(&m_n);
	}
	__forceinline void dec()
	{
		interlockedDecrement(&m_n);
	}
	__forceinline void acquire()
	{
		ChangeIf(0,-1);
	}
	__forceinline void release()
	{
		ChangeIf(-1,0);
	}
};

#endif // __SPINWAIT_H__
