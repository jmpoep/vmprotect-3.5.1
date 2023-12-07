#include "precompiled.h"

#ifdef CHECKED
const int PATTERN = 0xAA;
void *mynew(size_t s) 
{
	uint8_t *ptr = (uint8_t *)malloc(s + 8);
	if(ptr)
	{
		memset(ptr, PATTERN, s + 8);
		return ptr + 4;
	} else
	{
		abort();
	}
	return NULL;
}

void *operator new(size_t s) //throw(std::bad_alloc)
{
	return mynew(s);
}

void *operator new[](std::size_t s) //throw(std::bad_alloc)
{
	return mynew(s);
}

void *operator new(size_t s, const std::nothrow_t &) throw() 
{
	return mynew(s);
}

void *operator new[](size_t s, const std::nothrow_t &) throw() 
{
	return mynew(s);
}

void mydelete(void *p)
{
	if(p)
	{
		uint8_t *realp = (uint8_t *)p - 4;
#ifdef VMP_GNU
		size_t s =  malloc_usable_size(realp);
#else
		size_t s = _msize(realp);
#endif
		uint8_t *endp = realp + s;
		for(int i = 0; i < 4; i++)
		{
			bool needAbort = false;
			if(realp[i] != PATTERN)
			{
				std::cout << "Heap underflow at " << p << std::endl;
				needAbort = true;
			}
			if(*--endp != PATTERN)
			{
				std::cout << "Heap overflow at " << p << std::endl;
				needAbort = true;
			}
			if(needAbort)
			{
				abort();
			}
		}
		memset(realp, 0x55, s);
		free(realp);
	}

}
void operator delete(void *p) throw()
{
	mydelete(p);
}
void operator delete[](void *p) throw()
{
	mydelete(p);
}

void operator delete[](void *p, const std::nothrow_t &) throw() 
{
	mydelete(p);
}

void operator delete(void *p, const std::nothrow_t &) throw() 
{
	mydelete(p);
}

#endif