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

class LWSpinLock
{
#ifdef _DEBUG
public:
	static int &MaxIter() {
		static int n=0;
		return n;
	}
private:
#endif
	enum {
		MAXITER=10000
	};
public:
	LWSpinLock() : m_n(0)
	{
	}
	~LWSpinLock()
	{
	}
	bool tryLock()
	{
		const ALIGN MACHINE_INT test = 0;
		const ALIGN MACHINE_INT newVal = GetCurrentThreadId();
		return ( interlockedCompareExchange(&m_n, newVal, test) == test);
	}
	void lock()
	{
		const ALIGN MACHINE_INT test = 0;
		const ALIGN MACHINE_INT newVal = GetCurrentThreadId();
		if( m_n != newVal )	// re-entrant safe
		{
			for(int i=0;i<MAXITER;++i)
			{
				if(tryLock())
				{
#ifdef _DEBUG
					MaxIter() = ::std::max(MaxIter(),i);
#endif // _DEBUG
					break;
				}
				else do YieldProcessor();
					while(m_n != test);
			}
			if( m_n != newVal )
				throw ::std::runtime_error("Unexpected Spin-Wait deadlock detected");

		}
	}
	void unlock()
	{
		const ALIGN MACHINE_INT tid = GetCurrentThreadId();
		const ALIGN MACHINE_INT newVal = 0;
		if( m_n != tid )
			throw ::std::runtime_error("Unexpected thread-id in release");
		interlockedCompareExchange(&m_n, newVal, tid);
	}
private:
	LWSpinLock(const LWSpinLock &);
	LWSpinLock &operator=(const LWSpinLock &);
	volatile ALIGN MACHINE_INT m_n;
};

class LWSpinLocker
{
public:
	LWSpinLocker(LWSpinLock &lock) : m_lock(lock)
	{
		m_lock.lock();
	}
	~LWSpinLocker()
	{
		m_lock.unlock();
	}
private:
	LWSpinLocker(const LWSpinLocker &);
	LWSpinLocker &operator=(const LWSpinLocker &);
private:
	LWSpinLock &m_lock;
};

class LWTrySpinLocker
{
public:
	LWTrySpinLocker(LWSpinLock &lock) : m_lock(lock), m_bLocked(lock.tryLock())
	{
	}
	~LWTrySpinLocker()
	{
		if( locked() )
			m_lock.unlock();
	}
	bool locked()const { return m_bLocked; }
private:
	LWTrySpinLocker(const LWTrySpinLocker &);
	LWTrySpinLocker &operator=(const LWTrySpinLocker &);
private:
	LWSpinLock &m_lock;
	const bool	m_bLocked;
};


#endif // __SPINWAIT_H__
