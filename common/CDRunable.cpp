#include "stdafx.h"
#include "CDRunable.h"

CDRunable::CDRunable()
:_thread(NULL)
,_running(false)
{
}

CDRunable::~CDRunable()
{
}

bool CDRunable::Start()
{
	_thread = ::CreateThread(NULL, NULL, Runnable, this, CREATE_SUSPENDED, NULL);

	if (_thread)
	{
		::InterlockedIncrement((volatile LONG*)&_running);

		::ResumeThread(_thread);

		return true;
	}

	return false;
}

void CDRunable::Stop()
{
	if (_thread)
	{
		::InterlockedDecrement((volatile LONG*)&_running);

		::WaitForSingleObject(_thread, INFINITE);

		::CloseHandle(_thread);
		_thread = NULL;
	}
}

DWORD CDRunable::Runnable(LPVOID lPvoid)
{
	CDRunable *pThis = reinterpret_cast<CDRunable*>(lPvoid);

	do
	{
		if (!pThis->Run())
		{
			break;
		}

	} while(pThis->_running);

	return 0xdead;
}