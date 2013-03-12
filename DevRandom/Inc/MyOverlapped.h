#pragma once
#ifndef __MYOVERLAPPED_H__
#define __MYOVERLAPPED_H__

#include <memory>


struct MyOverlapped : public OVERLAPPED, public ::std::enable_shared_from_this<MyOverlapped>
{
	enum { BUFSIZE=256 };
	unsigned __int8 *buffer;
	unsigned __int8 *buffer_2;
	unsigned __int8 buffer1[BUFSIZE];
	unsigned __int8 buffer2[BUFSIZE];
	MyOverlapped()
	{
		reset();
	}
	MyOverlapped(const MyOverlapped &other) : OVERLAPPED(static_cast<const OVERLAPPED &>(other))
	{
		memcpy(buffer1,other.buffer1,sizeof(buffer1));
		memcpy(buffer2,other.buffer2,sizeof(buffer2));
		if( other.buffer == other.buffer1 )
		{
			buffer = buffer1;
			buffer_2 = buffer2;
		}
		else
		{
			buffer = buffer2;
			buffer_2 = buffer1;
		}

	}
	void reset()
	{
		memset(buffer1,'\0',sizeof(buffer1));
		memset(buffer2,'\0',sizeof(buffer2));
		OVERLAPPED *pOlp = static_cast<OVERLAPPED *>(this);
		memset(pOlp, '\0', sizeof(OVERLAPPED));
		buffer = buffer1;
		buffer_2 = buffer2;
	}
	void swapBuffers()
	{
		::std::swap(buffer,buffer_2);
		//if( buffer == buffer1 )
		//	buffer = buffer2;
		//else
		//	buffer = buffer1;
	}
	unsigned __int8 *Buffer1()
	{
		return buffer;
	}
	unsigned __int8 *Buffer2()
	{
		return buffer_2;
	}
};

typedef ::std::shared_ptr<MyOverlapped> MyOverlappedPtr;

#endif // __MYOVERLAPPED_H__
