#pragma once
#ifndef __SPINWAIT_H__
#define __SPINWAIT_H__

#ifdef _M_X64

#define interlockedCompareExchange(a,b,c) _InterlockedCompareExchange64(a,b,c)
#define interlockedIncrement(a) _InterlockedIncrement64(a)
#define interlockedDecrement(a) _InterlockedDecrement64(a)
typedef __int64 MACHINE_INT;	// processor specific natural word size integer

#else
#define interlockedCompareExchange(a,b,c) _InterlockedCompareExchange(a,b,c)
#define interlockedIncrement(a) _InterlockedIncrement(a)
#define interlockedDecrement(a) _InterlockedDecrement(a)
typedef long MACHINE_INT;

#endif

/// processor specific alignment 
#define ALIGN __declspec(align(MEMORY_ALLOCATION_ALIGNMENT))

#if 0
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
#endif

/// Light-weight spin-lock. 
/// This class is light-weight in the sense that it doesn't call Sleep()
/// or SwitchToThread(). This makes it suitable for use inside a thread pool
/// thread. Calling Sleep(), SwitchToThread()< or any other wait function in a 
/// thread pool thread risks a thread pool deadlock where all the pools threads are waiting 
/// an no threads are available to do work.
/// This class is also light-weight in the sense that it will only spin for a limited number of times 
/// before throwing a dead-locked runtime exception. 
class LWSpinLock
{
#ifdef _DEBUG
public:
	/// debugging helper to keep track of how bad the worst case spin-lock was.
	static int &MaxIter() {
		static int n=0;
		return n;
	}
private:
#endif
	enum {
		MAXITER=100	/// LWSpinLock will spin for no more than MAXITER iterations. At that point, it will throw an exception.
	};				/// if it throws an exception, then that indicates you need to address whatever is causing the thread pool threads
					/// to deadlock.
public:
	LWSpinLock() : m_n(0)
	{
	}
	~LWSpinLock()
	{
	}
	/// tryLock() pulls the interlockedCompareExchange() out into a method that LWTrySpinLocker() below can use
	/// tryLock() doesn't block.
	bool tryLock()
	{
		const ALIGN MACHINE_INT test = 0;
		const ALIGN MACHINE_INT newVal = GetCurrentThreadId();	/// use GetCurrentThread() as our lock value so the lock can be re-entrant.
		return ( interlockedCompareExchange(&m_n, newVal, test) == test);
	}
	/// lock the resource - this method will block until either a lock is obtained, or a deadlocked exception
	/// is thrown
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
					MaxIter() = ::std::max(MaxIter(),i);	/// keep track of the maximum number of iterations LWSpinLocks use.
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
	/// onlock the previously acquired lock
	void unlock()
	{
		const ALIGN MACHINE_INT tid = GetCurrentThreadId();
		const ALIGN MACHINE_INT newVal = 0;
		// make sure unlock() is called from the same thread that called lock()
		if( m_n != tid )
			throw ::std::runtime_error("Unexpected thread-id in release");
		interlockedCompareExchange(&m_n, newVal, tid);
	}
private:
	/// forbidden methods - no copying of this object
	LWSpinLock(const LWSpinLock &);
	LWSpinLock &operator=(const LWSpinLock &);
private:
	/// the integer used for locking
	volatile ALIGN MACHINE_INT m_n;
};

/// Class to provide Construction is resource aquisition design pattern
/// This class works with a LWSpinLock object to provide a lock in its constructor
/// and an unlock in its destructor.
class LWSpinLocker
{
public:
	/// acquire the lock
	LWSpinLocker(LWSpinLock &lock) : m_lock(lock)
	{
		m_lock.lock();
	}
	/// release the lock
	~LWSpinLocker()
	{
		m_lock.unlock();
	}
private:
	/// forbidden methods - no copying of this object
	LWSpinLocker(const LWSpinLocker &);
	LWSpinLocker &operator=(const LWSpinLocker &);
private:
	/// reference the the LWSpinLock object this object is controlling.
	LWSpinLock &m_lock;
};

/// This class is like LWSpinLocker above, except the constructor doesn't block.
/// instead it calls trylock() and stores the result in m_bLocked. If a lock is successfully
/// obtained, the destructor will release it. 
/// After you construct one of these, you must call locked() to see if you obtained the lock.
class LWTrySpinLocker
{
public:
	/// try to acquire a lock and store the result in m_bLocked.
	/// this constructor won't block
	LWTrySpinLocker(LWSpinLock &lock) : m_lock(lock), m_bLocked(lock.tryLock())
	{
	}
	/// release the lock is we successfully acquired it.
	~LWTrySpinLocker()
	{
		if( locked() )
			m_lock.unlock();
	}
	/// check the lock status
	bool locked()const { return m_bLocked; }
private:
	/// forbidden methods - no copying of this object
	LWTrySpinLocker(const LWTrySpinLocker &);
	LWTrySpinLocker &operator=(const LWTrySpinLocker &);
private:
	/// reference the the LWSpinLock object this object is controlling.
	LWSpinLock &m_lock;
	/// flag indicating if we acquired the lock.
	const bool	m_bLocked;
};


#endif // __SPINWAIT_H__
