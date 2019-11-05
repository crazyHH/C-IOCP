#include "stdafx.h"
#include "CDThreadPool.h"

JobItem* JobWork::pop_job()
{
	JobItem* job;

	job_lock.Lock();
	if (job_queue.empty())
	{
		job = NULL;
	}
	else
	{
		job = job_queue.front();
		job_queue.pop_front();
	}
	job_lock.Unlock();

	return job;
}

void JobWork::wait_job()
{
	WaitForSingleObject(job_event, 1000);
}

CDThreadPool::CDThreadPool(unsigned max)
:thread_max(max)
,thread_run(0)
,pool_run(false)
{
	thread_handle = new void*[thread_max];
	memset(thread_handle, sizeof(void*), thread_max);
}

CDThreadPool::~CDThreadPool()
{
	delete[] thread_handle;
}

/**
 *	函数:	run
 *
 *	功能:	开启所有线程
 *
 */
void CDThreadPool::run(unsigned run)
{
	pool_run = true;

	for (int i = 0; i != run; i++)
	{
		JobWork* work = new JobWork(this);
		job_work.push_back(work);

		thread_handle[i] = ::CreateThread(NULL, NULL, &thread_work, (LPVOID)work, CREATE_SUSPENDED, NULL);

		if (thread_handle[i])
		{
			thread_run++;
			::ResumeThread(thread_handle[i]);
		}
	}
}

/**
 *	函数:	stop
 *
 *	功能:	停止所有线程
 *
 */
void CDThreadPool::stop()
{
	pool_run = false;

	::WaitForMultipleObjects(thread_run, thread_handle, TRUE, INFINITE);

	for (int i = 0; i != thread_run; i++)
	{
		CloseHandle(thread_handle[i]);
		thread_handle[i] = NULL;
	}

	WorkArray::iterator iter = job_work.begin();
	for (iter; iter != job_work.end(); iter++)
	{
		delete (*iter);
	}
	job_work.clear();
}

/**
 *	函数:	wait_end
 *
 *	功能:	等待所有任务完成,需要设置auto_exit为true.
 *			线程在检测到任务队列为空后会自动退出
 *
 */
void CDThreadPool::wait_end()
{
	::WaitForMultipleObjects(thread_run, thread_handle, TRUE, INFINITE);

	for (int i = 0; i != thread_run; i++)
	{
		CloseHandle(thread_handle[i]);
		thread_handle[i] = NULL;
	}

	pool_run = false;
}

void CDThreadPool::push_job(int nId, Runable runable, void *param1, void *param2)
{
	JobItem* job = alloc_job(runable, param1, param2);
	JobWork* work = job_work[nId];
	
	bool bNotify = false;

	work->job_lock.Lock();
	if (work->job_queue.empty())
	{
		bNotify = true;
	}
	work->job_queue.push_back(job);
	work->job_lock.Unlock();

	if (bNotify)
	{
		SetEvent(work->job_event);
	}
}

DWORD CDThreadPool::thread_work(LPVOID lParam)
{
	JobWork*		pWork	= reinterpret_cast<JobWork*>(lParam);
	CDThreadPool*	pThis	= pWork->job_manager;

	while (pThis->pool_run)
	{
		JobItem* job = pWork->pop_job();
		
		if (job) {
			try 
			{
				job->runable(job->param1, job->param2);
				pThis->free_job(job);
			} 
			catch(...)
			{
				continue;
			}
		} else {
			pWork->wait_job();
		}
	}

	InterlockedDecrement((volatile LONG*)&pThis->thread_run);

	return 0xdead;
}

JobItem* CDThreadPool::alloc_job(Runable runable, void *param1, void *param2)
{
	return (new JobItem(runable, param1, param2));
}

void CDThreadPool::free_job(JobItem *job)
{
	delete job;
}