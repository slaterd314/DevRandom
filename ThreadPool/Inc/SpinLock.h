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
#define interlockedCompareExchange(a,b,c) _InterlockedCompareExchange(a,b,c)
#define interlockedIncrement(a) InterlockedIncrement(a)
#define interlockedDecrement(a) InterlockedDecrement(a)
typedef __int32 MACHINE_INT;
#define ALIGN __declspec(align(32))
#endif

class
ALIGN
SpinLock
{
protected:
	/// Change m_n to newVal if it equals test using InterlockedCompareExchange instruction.
	/// use a spin-wait to wait for the change - this call blocks until it succeeds
	volatile ALIGN MACHINE_INT m_n;
	__forceinline
	void ChangeIf(const int &test, const int &newVal)
	{
		const ALIGN MACHINE_INT t = test;
		const ALIGN MACHINE_INT n = newVal;
		// Note: Intel's example assembly code describing the PAUSE (_mm_pause) instruction that 
		// YieldProcessor() maps to is organized like this. First, attempt to change the value before going into the
		// spin-wait. In intel code, the spin-wait is organized to call PAUSE at the beginning of the loop
		// this is supposed to lend siginificant performance benefits to spin-wait loops making tghem exit much faster
		if(  interlockedCompareExchange(&m_n, n, t) == t  )
			;
		else do YieldProcessor();
			while( interlockedCompareExchange(&m_n, n, t) != t );
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
SpinWait : public SpinLock
{
public:
	SpinWait(bool bInitialLock) : SpinLock(bInitialLock){}
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
