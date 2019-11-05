/**
 *	模块: CDRunable
 *
 *	概述: 线程封装类
 *
 */
#ifndef _CDLIB_RUNABLE_H
#define _CDLIB_RUNABLE_H

#include "Define.h"

class CDRunable
{
public:
	CDRunable();
	virtual ~CDRunable();

	bool Start();
	void Stop();

	virtual bool Run() = 0;

private:

	static DWORD WINAPI Runnable(LPVOID lPvoid);

private:

	bool	_running;
	HANDLE	_thread;
};

#endif // _CDLIB_RUNABLE_H