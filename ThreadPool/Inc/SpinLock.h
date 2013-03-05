#pragma once
#ifndef __SPINWAIT_H__
#define __SPINWAIT_H__

#ifdef _M_X64
#define interlockedCompareExchange(a,b,c) _InterlockedCompareExchange64(a,b,c)
#define interlockedIncrement(a) _InterlockedIncrement64(a)
#define interlockedDecrement(a) _InterlockedDecrement64(a)
typedef __int64 MACHINE_INT;

#else
#define interlockedCompareExchange(a,b,c) _InterlockedCompareExchange(a,b,c)
#define interlockedIncrement(a) _InterlockedIncrement(a)
#define interlockedDecrement(a) _InterlockedDecrement(a)
typedef long MACHINE_INT;

#endif

#define ALIGN __declspec(align(MEMORY_ALLOCATION_ALIGNMENT))

class
ALIGN
SpinLock
{
	enum {
		SLEEP_AFTER_ITERATIONS=30,
		SWITCH_THREADS_AFTER_ITERATIONS=40
	};
	enum { foo = MEMORY_ALLOCATION_ALIGNMENT  };
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

		int nIter = 0;

		while(interlockedCompareExchange(&m_n, n, t) != t)
		{
			while(m_n != t)
			{
				if( nIter >=  SLEEP_AFTER_ITERATIONS )
				{
					Sleep(0);

					if( nIter >= SWITCH_THREADS_AFTER_ITERATIONS )
					{
						nIter = 0;
						SwitchToThread();
					}
				}
				++nIter;
				YieldProcessor();
			}
		}
	}
public:
	SpinLock(bool bInitialLock) : m_n(bInitialLock ? GetCurrentThreadId() : 0 )
	{
	}
	__forceinline void acquire()
	{
		if( m_n != (int)GetCurrentThreadId() )
			ChangeIf(0,GetCurrentThreadId());
	}
	__forceinline void release()
	{
		ChangeIf(GetCurrentThreadId(),0);
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
