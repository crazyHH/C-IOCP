#include "stdafx.h"
#include "CDTimer.h"
#include <MMSystem.h>

#pragma comment(lib, "winmm.lib")

CDTimerWheel::CDTimerWheel()
{
	m_pWheel1 = new CDWheel(WHEEL_COUNT_LEVEL1);
	m_pWheel2 = new CDWheel(WHEEL_COUNT_LEVEL2);
}

CDTimerWheel::~CDTimerWheel()
{
	delete m_pWheel1;
	delete m_pWheel2;
}

/* 开启定时器 */
bool CDTimerWheel::StartTimer()
{
	return Start();
}

/* 关闭定时器 */
void CDTimerWheel::StopTimer()
{
	Stop();
	RemoveAll();
}

/* 添加定时器 */
void* CDTimerWheel::AddTimer(int id,int peroid,int type,void* param,TimerProc proc)
{
	pWheelNode pNode = new WheelNode();

	if (pNode) {
		pNode->id = id;
		pNode->peroid = peroid;
		pNode->type = type;
		pNode->del = 0;
		pNode->param = param;
		pNode->proc = proc;
	}
	m_lock.Lock();
	ResetTimer(pNode, peroid);
	m_lock.Unlock();
	return pNode;
}

/* 删除定时器 */
void CDTimerWheel::RemoveTimer(void* timer)
{
	pWheelNode pNode = reinterpret_cast<pWheelNode>(timer);

	if (pNode) {
		/* 设置定时器待删除 */
		::InterlockedIncrement((volatile LONG*)&pNode->del);
	}
}

/* 重置定时器 */
void CDTimerWheel::ResetTimer(pWheelNode pNode, DWORD expires)
{
	if (expires < WHEEL_PRECISION*WHEEL_COUNT_LEVEL1) {
		int nStep = expires / WHEEL_PRECISION;

		/* 在精度范围内 */
		if (!nStep) {
			nStep = 1;
		}

		InsertTimer(pNode, m_pWheel1, nStep);

	} else /*if(peroid < WHEEL_PRECISION*WHEEL_COUNT_LEVEL1*WHEEL_COUNT_LEVEL2)*/ {
		int nStep = expires / (WHEEL_PRECISION*WHEEL_COUNT_LEVEL1);

		if (nStep >= WHEEL_COUNT_LEVEL2) {
			nStep = WHEEL_COUNT_LEVEL2 - 1;
		}

		pNode->expires = expires % (WHEEL_PRECISION*WHEEL_COUNT_LEVEL1);

		InsertTimer(pNode, m_pWheel2, nStep);
	}/* else if {*/
}

/* 定时器插入到指定时间轮插槽 */
void CDTimerWheel::InsertTimer(pWheelNode pNode, CDWheel* pWheel, int nStep)
{
	int nEnd = pWheel->m_nCrtPos + nStep;
	if (nEnd >= pWheel->m_nWheelNum) {
		nEnd -= pWheel->m_nWheelNum;
	}

	LinkList(&(pWheel->m_pWheelNode[nEnd]), pNode);
}

/* 清理所有定时器 */
void CDTimerWheel::RemoveAll()
{
	RemoveWheel(m_pWheel1);
	RemoveWheel(m_pWheel2);
}

void CDTimerWheel::RemoveWheel(CDWheel* pWheel)
{
	for (int i = 0; i != pWheel->m_nWheelNum; i ++)
	{
		pWheelNode pNode = pWheel->m_pWheelNode[i].next;
		pWheelNode pNext;
		while (pNode)
		{
			pNext = pNode->next;
			delete pNode;
			if (pNext == &(pWheel->m_pWheelNode[i])) {
				break;
			}
			pNode = pNext;
		}
	}
}

/* 时间轮调度 */
bool CDTimerWheel::Run()
{
	m_dwTime = timeGetTime();

	m_lock.Lock();

	/* 定时器调度 */
	pWheelNode pNode = Tick();
	pWheelNode pNext;

	/* 采用双向链表设计，这里需要判断下一个节点是不是根节点 */
	while (pNode && pNode->id)
	{
		pNext = pNode->next;
		if (pNode->del) {
			UnlinkList(pNode);
			delete pNode;
		} else {
			/* 调用时间回调 */
			pNode->proc(pNode->id, pNode->param);
			UnlinkList(pNode);
			if (pNode->type == TIMER_TYPE_RESET) {
				pNode->expires = pNode->peroid;
				ResetTimer(pNode, pNode->expires);
			} else {
				delete pNode;
			}
		}
		pNode = pNext;
	}

	m_lock.Unlock();

	DWORD dwTick = timeGetTime() - m_dwTime;

	if (dwTick < WHEEL_PRECISION) {
		timeBeginPeriod(1);
		Sleep(WHEEL_PRECISION - dwTick);
		timeEndPeriod(1);
	}

	return true;
}

/* 获取下一个刻度时间槽的定时器 */
pWheelNode CDTimerWheel::Tick()
{
	pWheelNode pNode = NULL;
	/* 先遍历第一级时间轮 */
	if (m_pWheel1->m_nCrtPos == m_pWheel1->m_nWheelNum) {
		m_pWheel1->m_nCrtPos = 0;
		if (m_pWheel2->m_nCrtPos == m_pWheel2->m_nWheelNum) {
			m_pWheel2->m_nCrtPos = 0;
		}
		/* 遍历第二级时间轮上某一刻度所有定时器，并加入第一级时间轮重新计时 */
		pWheelNode pRoot = &(m_pWheel2->m_pWheelNode[m_pWheel2->m_nCrtPos]);
		pWheelNode pInst = pRoot->next;
		pWheelNode pNext = NULL;
	
		while (pInst && pInst != pRoot)
		{
			pNext = pInst->next;

			pInst->next = 0;
			pInst->prev = 0;

			ResetTimer(pInst, pInst->expires);

			pInst = pNext;
		}

		pRoot->next = 0;
		pRoot->prev = 0;

		m_pWheel2->m_nCrtPos++;
	}
	
	/* 获取第一个定时器节点 */
	pNode = &(m_pWheel1->m_pWheelNode[m_pWheel1->m_nCrtPos++]);
	pNode = pNode->next;

	return pNode;
}

/* 定时器节点链表操作 */
void CDTimerWheel::LinkList(pWheelNode pTarget, pWheelNode pInsert)
{
	if (!pTarget->next) {
		/* 只有根节点 */
		pTarget->next = pInsert;
		pTarget->prev = pInsert;

		pInsert->prev = pTarget;
		pInsert->next = pTarget;
	} else {
		pInsert->next = pTarget;
		pInsert->prev = pTarget->prev;

		pTarget->prev->next = pInsert;
		pTarget->prev = pInsert;
	}
}

void CDTimerWheel::UnlinkList(pWheelNode pNode)
{
	if (pNode->prev == pNode->next) {
		/* 只有两个节点 */
		pNode->next->prev = NULL;
		pNode->next->next = NULL;
	} else {
		pNode->prev->next = pNode->next;
		pNode->next->prev = pNode->prev;
	}
	pNode->next = NULL;
	pNode->prev = NULL;
}