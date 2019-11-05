#pragma once

#include <list>
#include <vector>
#include <windows.h>

#define MAX_THREAD_NUM	60

typedef	void (CALLBACK* Runable)(void*, void*);

class JobItem;
class JobWork;
class ThreadPool;

typedef std::list<JobItem*>	JobQueue;
typedef std::vector<JobWork*> WorkArray;

class CPoolLock
{
public:
 
	CPoolLock()
	{
		InitializeCriticalSection(&m_cs);
	};

	~CPoolLock()
	{
		DeleteCriticalSection(&m_cs);
	};

	VOID Unlock()
	{
		LeaveCriticalSection(&m_cs);
	};

	void Lock()
	{
		EnterCriticalSection(&m_cs);
	};

private:
	CRITICAL_SECTION	m_cs;
};

class JobItem
{
private:
	JobItem(Runable run, void *p1, void *p2):runable(run),param1(p1),param2(p2){};
	~JobItem() {};

	Runable		runable;
	void		*param1;
	void		*param2;

	friend class JobWork;
	friend class CDThreadPool;
};

class JobWork
{
private:
	JobWork(CDThreadPool* pThis) : job_manager(pThis) { job_event = CreateEvent(0, 0, 0, 0); };
	~JobWork() { CloseHandle(job_event); };

	JobItem*	pop_job();
	void		wait_job();

private:
	/* �߳�������� */
	JobQueue	job_queue;
	/* �̳߳�ͬ���� */
	CPoolLock	job_lock;
	/* �ź���֪ͨ���� */
	HANDLE		job_event;
	/* ͬ���̳߳� */
	CDThreadPool* job_manager;

	friend class CDThreadPool;
};

class CDThreadPool
{
public:
	CDThreadPool(unsigned max=MAX_THREAD_NUM);
	~CDThreadPool();

	void		run(unsigned run=MAX_THREAD_NUM);
	void		stop();
	void		wait_end();

	void		push_job(int nThread, Runable runable, void *param1, void *param2);

private:
	static DWORD WINAPI thread_work(LPVOID lParam);

	JobItem*	alloc_job(Runable runable, void *param1, void *param2);
	void		free_job(JobItem *job);

private:
	unsigned	thread_max;		/* �̳߳���������߳��� */

	unsigned	thread_run;		/* �̳߳ص�ǰ�����߳��� */

	void**		thread_handle;	/* �����߳̾�� */

	bool		pool_run;		/* �̳߳��Ƿ����� */

	WorkArray	job_work;		/* �̳߳ع����� */
};