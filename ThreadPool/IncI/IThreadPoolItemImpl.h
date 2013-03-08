#pragma once
#ifndef __ITHREADPOOLITEMIMPL_H__
#define __ITHREADPOOLITEMIMPL_H__

template <class C>
class IThreadPoolItemImpl : public C
{
	class CThreadPool *m_pool;
public:
	IThreadPoolItemImpl();
	IThreadPoolItemImpl(class CThreadPool *p);
	~IThreadPoolItemImpl();
	virtual class CThreadPool *pool()
	{
		return m_pool;
	}
protected:
	friend class CThreadPool;
	virtual void setPool(class CThreadPool *p);
};


#endif // __ITHREADPOOLITEMIMPL_H__
