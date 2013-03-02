#pragma once
#ifndef __ITHREADPOOLITEM_H__
#define __ITHREADPOOLITEM_H__

#include <memory>

class
__declspec(novtable)
IThreadPoolItem : public ::std::enable_shared_from_this<IThreadPoolItem>
{
public:
	virtual class IThreadPool *pool()=0;
	virtual ~IThreadPoolItem() {}
protected:
	virtual void setPool(IThreadPool *f)=0;
};

typedef ::std::shared_ptr<IThreadPoolItem> IThreadPoolItemPtr;

#endif // __ITHREADPOOLITEM_H__
