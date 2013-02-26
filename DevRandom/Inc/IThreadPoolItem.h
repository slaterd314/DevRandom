#pragma once
#ifndef __ITHREADPOOLITEM_H__
#define __ITHREADPOOLITEM_H__

class
__declspec(novtable)
IThreadPoolItem
{
public:
	virtual class IThreadPool *pool()=0;
	virtual ~IThreadPoolItem() {}
protected:
	virtual void setPool(IThreadPool *f)=0;
};


#endif // __ITHREADPOOLITEM_H__
