#pragma once
#ifndef __ITHREADPOOLITEMIMPL_H__
#define __ITHREADPOOLITEMIMPL_H__

template <class C>
class IThreadPoolItemImpl : public C
{
	IThreadPool *m_pool;
public:
	IThreadPoolItemImpl();
	IThreadPoolItemImpl(class CThreadPool *p);
	~IThreadPoolItemImpl();
	virtual IThreadPool *pool()
	{
		return m_pool;
	}
protected:
	friend class CThreadPool;
	virtual void setPool(IThreadPool *p);
};


#endif // __ITHREADPOOLITEMIMPL_H__
