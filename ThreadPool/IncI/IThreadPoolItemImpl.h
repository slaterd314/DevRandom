#pragma once
#ifndef __ITHREADPOOLITEMIMPL_H__
#define __ITHREADPOOLITEMIMPL_H__

#include "CThreadPool.h"

template <class C>
class IThreadPoolItemImpl : public C
{
	class CThreadPool *m_pool;
public:
	IThreadPoolItemImpl();
	IThreadPoolItemImpl(class CThreadPool *p);
	~IThreadPoolItemImpl();
	virtual class CThreadPool *pool();
protected:
	friend class CThreadPool;
	virtual void setPool(class CThreadPool *p);
	void setHandle(void *h);
	void *getHandle();
private:
	void *	m_handle;
};


template<class C>
inline
IThreadPoolItemImpl<C>::IThreadPoolItemImpl() : m_pool(NULL)
{
	//TRACE(TEXT(">>>%S::IThreadPoolItemImpl(), this=0x%x.\n"), typeid(*this).name() , this); 
}

template<class C>
inline
IThreadPoolItemImpl<C>::IThreadPoolItemImpl(class CThreadPool *p) : m_pool(p)
{
	//TRACE(TEXT(">>>%S::IThreadPoolItemImpl(p), this=0x%x.\n"), typeid(*this).name() , this); 
	p->insertItem(this);
}

template<class C>
inline
IThreadPoolItemImpl<C>::~IThreadPoolItemImpl()
{
	//TRACE(TEXT("<<<%S::~IThreadPoolItemImpl(), this=0x%x.\n"), typeid(*this).name() , this); 
	static_cast<CThreadPool *>(pool())->removeItem(this);
}

template<class C>
inline
void
IThreadPoolItemImpl<C>::setPool(CThreadPool *p)
{
	if( m_pool == NULL )
	{
		static_cast<CThreadPool *>(p)->insertItem(this);
		m_pool = p;
	}
}

template<class C>
inline
CThreadPool *
IThreadPoolItemImpl<C>::pool()
{
	return m_pool;
}

template<class C>
inline
void
IThreadPoolItemImpl<C>::setHandle(void *h)
{
	m_handle = h;
}

template<class C>
inline
void *
IThreadPoolItemImpl<C>::getHandle()
{
	return m_handle;
}


#endif // __ITHREADPOOLITEMIMPL_H__
