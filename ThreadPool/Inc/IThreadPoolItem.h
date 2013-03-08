#pragma once
#ifndef __ITHREADPOOLITEM_H__
#define __ITHREADPOOLITEM_H__

#include <memory>

/// Base thread pool item that Work,wait,IoCompletion, & timer 
/// objects derive from. 
class
__declspec(novtable)
IThreadPoolItem : public ::std::enable_shared_from_this<IThreadPoolItem>
{
public:
	/// retreive the thread pool that created this object
	virtual class IThreadPool *pool()=0;
	virtual ~IThreadPoolItem() {}
	typedef ::std::shared_ptr<class IThreadPoolItem> Ptr;
protected:
	virtual void setPool(class CThreadPool *f)=0;
};


#endif // __ITHREADPOOLITEM_H__
