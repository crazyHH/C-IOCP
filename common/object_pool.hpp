#ifndef _OBJECT_POOL_HPP
#define _OBJECT_POOL_HPP

#include "Define.h"
#include <list>

template<class type_name, class type_size = size_t>
class object_pool
{
public:
	object_pool(type_size max_cache=100);
	~object_pool();

	type_name*		construct();
	void			destroy(type_name* ptr);
	bool			empty();
	void			clear();

private:
	void			lock();
	void			unlock();

	type_size				pool_max;
	std::list<type_name*>	pool_ptr;
	CRITICAL_SECTION		pool_lock;
};

template<class type_name, class type_size>
object_pool<type_name, type_size>::object_pool(type_size max_cache)
	:pool_max(max_cache)
{
	::InitializeCriticalSection(&pool_lock);
}

template<class type_name, class type_size>
object_pool<type_name, type_size>::~object_pool()
{
	clear();
	::DeleteCriticalSection(&pool_lock);
}

template<class type_name, class type_size>
type_name* object_pool<type_name, type_size>::construct()
{
	type_name* ptr = NULL;
	lock();
	if(pool_ptr.empty())
	{
		unlock();
		ptr = new type_name();
	}
	else
	{
		ptr = pool_ptr.front();
		pool_ptr.pop_front();
		unlock();
	}
	return ptr;
}

template<class type_name, class type_size>
void object_pool<type_name, type_size>::destroy(type_name* ptr)
{
	lock();
	if(pool_ptr.size() >= pool_max)
	{
		unlock();
		delete ptr;
	}
	else
	{
		pool_ptr.push_back(ptr);
		unlock();
	}
}

template<class type_name, class type_size>
bool object_pool<type_name, type_size>::empty()
{
	bool isEmpty = false;
	lock();
	isEmpty = pool_ptr.empty();
	unlock();

	return isEmpty;
}

template<class type_name, class type_size>
void object_pool<type_name, type_size>::clear()
{
	std::list<type_name*>::iterator start = pool_ptr.begin();
	std::list<type_name*>::iterator end   = pool_ptr.end();

	for(; start != end; start++)
	{
		delete (*start);
	}

	pool_ptr.clear();
}

template<class type_name, class type_size>
void object_pool<type_name, type_size>::lock()
{
	::EnterCriticalSection(&pool_lock);
}

template<class type_name, class type_size>
void object_pool<type_name, type_size>::unlock()
{
	::LeaveCriticalSection(&pool_lock);
}

#endif //_OBJECT_POOL_HPP