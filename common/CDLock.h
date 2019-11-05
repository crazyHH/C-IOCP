/** 
 *	模块: CDLock.h
 *	
 *	概述: 封装了临界区锁
 *
 */
#pragma once

#include "Define.h"


class CDLock
{
public:
 
	CDLock()
	{
		InitializeCriticalSection(&m_cs);
	}

	~CDLock()
	{
		DeleteCriticalSection(&m_cs);
	}

	VOID Unlock()
	{
		LeaveCriticalSection(&m_cs);
	}

	void Lock()
	{
		EnterCriticalSection(&m_cs);
	}


private:
	CRITICAL_SECTION	m_cs;

	friend class CLock;
};

class CLock
{
public:
 
	CLock(CRITICAL_SECTION& cs, const TCHAR *pszFunc)
	{
		m_pcs = &cs;
		Lock();
	}

	CLock(CDLock* pLock, const TCHAR *pszFunc)
	{
		m_pcs = &pLock->m_cs;
		Lock();
	}

	~CLock()
	{
		Unlock();
	}

	void Lock()
	{
		EnterCriticalSection(m_pcs);
	}

	VOID Unlock()
	{
		LeaveCriticalSection(m_pcs);
	}

private:
	CRITICAL_SECTION*	m_pcs;
};