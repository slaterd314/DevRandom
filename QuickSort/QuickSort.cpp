// QuickSort.cpp : Defines the entry point for the console application.
//


#include "stdafx.h"
#include "IWork.h"
#include "IThreadPool.h"
#include "SpinLock.h"
#include <functional>
#include <algorithm>
#include <vector>
#include <iterator>
#include <tchar.h>
#include <intrin.h>
  
#pragma intrinsic(__rdtsc)

::std::vector<int> arr;
const size_t nArrSize = 1000000;

static
ALIGN
MACHINE_INT
nWorkPackets=0;

template< typename BidirectionalIterator, typename Compare >
void quick_sort( BidirectionalIterator first, BidirectionalIterator last, Compare cmp ) {
    if( first != last ) {
		
        BidirectionalIterator left  = first;
        BidirectionalIterator right = last;
        BidirectionalIterator pivot = left++;
 
        while( left != right ) {
            if( cmp( *left, *pivot ) ) {
                ++left;
            } else {
                while( (left != right) && cmp( *pivot, *right ) )
                    --right;
                std::iter_swap( left, right );
            }
        }
 
        --left;
        std::iter_swap( pivot, left );
 
		// TRACE(TEXT("First1 %d, Last1 %d\tFirst2 %d, Last2 %d\n"), (&(*first))-(&(arr[0])), (&(*left))-(&(arr[0])), (&(*right))-(&(arr[0])), (&(*last))-(&(arr[0])));
        quick_sort( first, left, cmp );
        quick_sort( right, last, cmp );
    }
}


template< typename BidirectionalIterator, typename Compare >
IWork::FuncPtr
make_quick_sort( BidirectionalIterator first, BidirectionalIterator last, Compare cmp, IWork::Ptr work) {
	return [=](PTP_CALLBACK_INSTANCE , IWork *pWork) {
    if( (last-first) > 500 ) {
		
        BidirectionalIterator left  = first;
        BidirectionalIterator right = last;
        BidirectionalIterator pivot = left++;
 
        while( left != right ) {
            if( cmp( *left, *pivot ) ) {
                ++left;
            } else {
                while( (left != right) && cmp( *pivot, *right ) )
                    --right;
                std::iter_swap( left, right );
            }
        }
 
        --left;
        std::iter_swap( pivot, left );
 
        // create a new work packet to process first to left
		interlockedIncrement(&nWorkPackets);
		IWork::Ptr new_work = pWork->pool()->newWork([=](PTP_CALLBACK_INSTANCE , IWork *){});
		new_work->setWork(make_quick_sort(first, left, cmp, new_work));
		new_work->SubmitWork();

		// reset first to right and re-submit this work packet to process from right to last
		const_cast<BidirectionalIterator &>(first) = right;		
 		pWork->SubmitWork();
		// quick_sort( first, left, cmp );
       // quick_sort( right, last, cmp );
    }
	else if( last != first )
	{
		::std::sort(first,(last+1));
		if( work.get() )
			interlockedDecrement(&nWorkPackets);
	}
	else
	{
		if( work.get() )
			interlockedDecrement(&nWorkPackets);
		//const_cast<IWork::Ptr &>(work).reset();
	}
	};
}
 

template< typename BidirectionalIterator >
    inline void sort( BidirectionalIterator first, BidirectionalIterator last ) {
		// TRACE(TEXT("First %d, Last %d\tFirst2 %d, Last2 %d\n"), (&(*first))-(&(arr[0])), (&(*last))-(&(arr[0])));
        quick_sort( first, last,
                std::less_equal< typename ::std::iterator_traits< BidirectionalIterator >::value_type >()
                );
    }

static IThreadPool::Ptr pPool = IThreadPool::newPool();

template< typename BidirectionalIterator >
    inline void sortM( BidirectionalIterator first, BidirectionalIterator last ) {

		if( pPool.get() )
		{
			IWork::Ptr new_work = pPool->newWork([=](PTP_CALLBACK_INSTANCE , IWork *){});
			interlockedIncrement(&nWorkPackets);
			new_work->setWork(make_quick_sort(first, last, std::less_equal< typename ::std::iterator_traits< BidirectionalIterator >::value_type >(), new_work));
			new_work->SubmitWork();

			while( interlockedCompareExchange(&nWorkPackets,0,0) != 0 )
			{
				YieldProcessor();
			}
		}
    }

static
int 
mycmp(void *a, void *b)
{
	return (*(int*)a)-(*(int*)b); 
}

int _tmain(int argc, _TCHAR* argv[])
{
	// int *arr = new int[nArrSize];
	
	arr.reserve(nArrSize);
	for( int i=0; i<nArrSize; ++i )
	{
		unsigned int n=0;
		rand_s((unsigned int *)&n);
		arr.push_back(n);
	}

	::std::vector<int> arr2 = arr;


	FILE *fp = fopen("e:\\Before.txt","w");
	if( fp )
	{
		for( int i=0; i<nArrSize; ++i )
			fprintf(fp,"%d\n",arr[i]);
		fclose(fp);
	}

	_tprintf(TEXT("Hit a key to sort\n"));

	TCHAR buf[32] = {0};
	_getws_s(buf);


	unsigned __int64 i1 = __rdtsc();

	::sort(arr.begin(),--(arr.end()));

	unsigned __int64 i2 = __rdtsc();

	_tprintf(TEXT("Hit a key to Msort\n"));

	_getws_s(buf);


	sortM(arr2.begin(),--(arr2.end()));

	unsigned __int64 i3 = __rdtsc();

	fp = fopen("e:\\After.txt","w");
	if( fp )
	{
		for( int i=0; i<nArrSize; ++i )
			fprintf(fp,"%d\n",arr[i]);
		fclose(fp);
	}

	fp = fopen("e:\\After2.txt","w");
	if( fp )
	{
		for( int i=0; i<nArrSize; ++i )
			fprintf(fp,"%d\n",arr2[i]);
		fclose(fp);
	}


	unsigned __int64 di1 = ((i2 >= i1) ? (i2-i1) : (((unsigned __int64)(-1)) - i1) + i2);
	unsigned __int64 di2 = ((i3 >= i2) ? (i3-i2) : (((unsigned __int64)(-1)) - i2) + i3);

	_tprintf(TEXT("Single Threaded Sort took %I64u cycles.\nMulti Threaded Sort took %I64u cycles.\n"),di1, di2);

	_getws_s(buf);

	return 0;
}




#if 0
static void swap(void *x, void *y, size_t l) {
   char *a = (char *)x, *b = (char *)y, c;
   while(l--) {
      c = *a;
      *a++ = *b;
      *b++ = c;
   }
}

static
ALIGN
MACHINE_INT
nWorkItems = 0;

template< class T, class Cmp >
IWork::FuncPtr
makeSort(T *arr, int begin, int end, IWork::Ptr work)
{
	return [=](PTP_CALLBACK_INSTANCE , IWork *pWork) {
		if ( (end-begin) > 0 ) 
		{
			  T *pivot = arr + begin;
			  int l = begin + 1;
			  int r = end;
			  while(l < r) {
				 if ( Cmp()(*(arr+l),*pivot) ) {
					++l;
				 } else {
					--r;
					::std::swap(*(arr+l), *(arr+r));
				 }
			  }
			  --l;
			  ::std::swap(*(arr+begin), *(arr+l));
			  
			  // pWork->setWork(makeSort(arr, size, cmp, begin, end, work));
			  
			  // split into 2 sub-ranges. process begin to l with this work packet
			  // by changing end
			  const_cast<int &>(end) = l;
			  // create a new work packet to process from r to end
			  IWork::Ptr new_work = pWork->pool()->newWork([=](PTP_CALLBACK_INSTANCE , IWork *){});
			  interlockedIncrement(&nWorkItems);
			  new_work->setWork(makeSort<T, Cmp>(arr, r, end,new_work));

			  // submit the 2 new work packets.
			  pWork->SubmitWork();
			  new_work->SubmitWork();
		}
		else
		{
			const_cast<IWork::Ptr &>(work).reset();
			interlockedDecrement(&nWorkItems);
		}
	};
}

template< class T, class Cmp>
void
sort(T *arr, int begin, int end)
{
	IThreadPool::Ptr pPool = IThreadPool::newPool();
	if( pPool.get() )
	{
		IWork::Ptr work = pPool->newWork([=](PTP_CALLBACK_INSTANCE , IWork *){});
		interlockedIncrement(&nWorkItems);
		work->setWork(makeSort<int, Cmp >(arr,begin,end,work));
		work->SubmitWork();

		while( interlockedCompareExchange(&nWorkItems,0,0) != 0 )
		{
			Sleep(1);
		}
	}
}

#endif // 0