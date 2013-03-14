#pragma once
#ifndef __SPINWAIT_H__
#define __SPINWAIT_H__

#ifdef _DEBUG
#ifndef TRACK_SPIN_COUNTS
#define TRACK_SPIN_COUNTS 1
#endif
#endif

// #define TRACK_SPIN_COUNTS 1
#undef TRACK_SPIN_COUNTS

#ifdef TRACK_SPIN_COUNTS
#define RECORD_MAX(a,b) a = ::std::max(a,b); \
	++(Calls()); \
	Sum() += b

#include <intrin.h>
  
#pragma intrinsic(__rdtsc)


#define DECLARE_TIME_ACQUIRE_LOCK() \
public: \
	static unsigned __int64 &MaxDt() { \
		static unsigned __int64 n=0; \
		return n; \
	} \
	static unsigned __int64 &Calls() { \
		static unsigned __int64 n=0; \
		return n; \
	}; \
	static unsigned __int64 &Sum() { \
		static unsigned __int64 n=0; \
		return n; \
	}


#define BEGIN_TIME_ACQUIRE_LOCK() unsigned __int64 i1 = __rdtsc()

#define END_TIME_ACQUIRE_LOCK() \
unsigned __int64 i2 = __rdtsc(); \
unsigned __int64 dt; \
if( i2 >= i1 ) \
	dt = i2-i1; \
else \
	dt = (((unsigned __int64)(-1)) - i1) + i2; /* overflowed the unsigned __int64 */ \
RECORD_MAX(MaxDt(),dt)


#else

#define RECORD_MAX(a,b) 
#define DECLARE_TIME_ACQUIRE_LOCK() 
#define BEGIN_TIME_ACQUIRE_LOCK() 
#define END_TIME_ACQUIRE_LOCK() 

#endif



#ifdef _M_X64
typedef __int64 MACHINE_INT;	// processor specific natural word size integer

#define interlockedExchange(a,b)			_InterlockedExchange64(a,b)
#define interlockedCompareExchange(a,b,c)	_InterlockedCompareExchange64(a,b,c)
#define interlockedIncrement(a)				_InterlockedIncrement64(a)
#define interlockedDecrement(a)				_InterlockedDecrement64(a)

#else
typedef long MACHINE_INT;

#define interlockedExchange(a,b)			_InterlockedExchange(a,b)
#define interlockedCompareExchange(a,b,c)	_InterlockedCompareExchange(a,b,c)
#define interlockedIncrement(a)				_InterlockedIncrement(a)
#define interlockedDecrement(a)				_InterlockedDecrement(a)

#endif

// processor specific alignment 
#define ALIGN __declspec(align(MEMORY_ALLOCATION_ALIGNMENT))

///<summary>
/// Light weight spin lock. 
/// This class is light weight in the sense that it doesn't call Sleep()
/// or SwitchToThread(). This makes it suitable for use inside a thread pool
/// thread. Calling Sleep(), SwitchToThread() or any other wait function in a 
/// thread pool thread risks a thread pool deadlock where all the pools threads are waiting 
/// an no threads are available to do work.
/// This class is also light-weight in the sense that it will only spin for a limited number of times 
/// before throwing a dead-locked runtime exception. 
/// </summary>
class LWSpinLock
{
	DECLARE_TIME_ACQUIRE_LOCK();
private:
	enum {
		MAXITER=100	/// LWSpinLock will spin for no more than MAXITER iterations. At that point, it will throw an exception.
	};				/// if it throws an exception, then that indicates you need to address whatever is causing the thread pool threads
					/// to deadlock.
public:
	LWSpinLock() : m_n(0), m_nCpus(GetMinYieldIters())
	{
	}
	~LWSpinLock()
	{
	}
	/// tryLock() pulls the interlockedCompareExchange() out into a method that LWTrySpinLocker() below can use
	/// tryLock() doesn't block.
	bool tryLock(const MACHINE_INT &n)
	{
		static const ALIGN MACHINE_INT test = 0;
		const ALIGN MACHINE_INT newVal = n;	/// use GetCurrentThread() as our lock value so the lock can be re-entrant.
		return ( (m_n == test) &&			// Only call interlockedCompareExchange if a simple comparison indicates we can get the lock
				interlockedCompareExchange(&m_n, newVal, test) == test);
	}
	inline
	void yield(const int & /*nIter*/)
	{
		YieldProcessor();
	}
	bool TryLock(const MACHINE_INT &tid)
	{
		int nIter = 1;
		if( m_n != tid )	// re-entrant safe
		{
			for(;	!tryLock(tid) && 
					nIter < m_nCpus;	// only spin until we would need to call Sleep()
					++nIter)
			{
				yield(nIter);
			}
		}
		RECORD_MAX(MaxDt(),static_cast<unsigned __int64>(nIter)); /// keep track of the maximum number of iterations LWSpinLocks use.
	}
	/// lock the resource - this method will block until either a lock is obtained, or a deadlocked exception
	/// is thrown
	void lock(const MACHINE_INT &tid)
	{
		int nIter = 1;
		if( m_n != tid )	// re-entrant safe
		{
			for(;!tryLock(tid);++nIter)
			{
				yield(nIter);
			}
		}
		RECORD_MAX(MaxDt(),static_cast<unsigned __int64>(nIter)); /// keep track of the maximum number of iterations LWSpinLocks use.
	}
	/// onlock the previously acquired lock
	void unlock(const MACHINE_INT &tid)
	{
		static const ALIGN MACHINE_INT newVal = 0;
		// make sure unlock() is called from the same thread that called lock()
		if( m_n != tid )
			throw ::std::runtime_error("Unexpected thread-id in release");
		interlockedCompareExchange(&m_n, newVal, tid);
	}
	static DWORD GetMinYieldIters()
	{
		static const DWORD dwMinYieldIters = ((numCpus() < 8) ? 8 : numCpus());
		return dwMinYieldIters;
	}
private:
	static DWORD numCpus()
	{
		SYSTEM_INFO info = {0};
		GetSystemInfo(&info);
		return info.dwNumberOfProcessors;
	}
	/// forbidden methods - no copying of this object
	LWSpinLock(const LWSpinLock &);
	LWSpinLock &operator=(const LWSpinLock &);
private:
	/// the integer used for locking
	ALIGN MACHINE_INT m_n;
	int m_nCpus;
};

/// Class to provide Construction is resource aquisition design pattern
/// This class works with a LWSpinLock object to provide a lock in its constructor
/// and an unlock in its destructor.
class LWSpinLocker
{
	const ALIGN MACHINE_INT tid;
	DECLARE_TIME_ACQUIRE_LOCK();
public:
	/// acquire the lock
	LWSpinLocker(LWSpinLock &lock) : m_lock(lock), tid(GetCurrentThreadId())
	{
		BEGIN_TIME_ACQUIRE_LOCK();
		m_lock.lock(tid);
		END_TIME_ACQUIRE_LOCK();
	}
	/// release the lock
	~LWSpinLocker()
	{
		m_lock.unlock(tid);
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
	DECLARE_TIME_ACQUIRE_LOCK();
public:
	/// try to acquire a lock and store the result in m_bLocked.
	/// this constructor won't block
	LWTrySpinLocker(LWSpinLock &lock) : m_lock(lock), tid(GetCurrentThreadId())
	{
		BEGIN_TIME_ACQUIRE_LOCK();
		m_bLocked = lock.TryLock(tid);
		END_TIME_ACQUIRE_LOCK();
	}
	/// release the lock is we successfully acquired it.
	~LWTrySpinLocker()
	{
		if( locked() )
			m_lock.unlock(tid);
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
	bool	m_bLocked;
	const MACHINE_INT tid;
};


#endif // __SPINWAIT_H__
