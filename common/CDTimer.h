#pragma once
#include "../Common/CDLock.h"
#include "CDRunable.h"

//定时器类型
#define TIMER_TYPE_ONCE		0
#define TIMER_TYPE_RESET	1

//每个时间轮的轮子数
#define WHEEL_COUNT_LEVEL1	1024
#define WHEEL_COUNT_LEVEL2	256

//每个轮子代表的时间精度(毫秒)
#define WHEEL_PRECISION		50

typedef void (CALLBACK* TimerProc)(int, void*);

//定时器时间节点
typedef struct WHEELNODE
{
	int			id;		// 定时器id
	int			peroid;	// 时间间隔
	int			expires;// 超时时间
	int			type;	// 定时器类型
	int			del;	// 是否删除定时器
	void*		param;	// 回调函数参数
	TimerProc	proc;	// 回调函数
	WHEELNODE*	prev;	// 链表上一个节点
	WHEELNODE*	next;	// 链表下一个节点
}WheelNode,*pWheelNode;

//时间轮
struct CDWheel
{
	CDWheel(int num):m_nCrtPos(0),m_nWheelNum(num)
	{
		/* 初始化时间轮根节点 */
		m_pWheelNode = new WheelNode[m_nWheelNum];
		ZeroMemory(m_pWheelNode, sizeof(WheelNode)*m_nWheelNum);
	};

	~CDWheel()
	{
		delete[] m_pWheelNode;
	};

	int	m_nCrtPos;	//时间轮当前位置
	int	m_nWheelNum;//时间轮插槽总数
	pWheelNode m_pWheelNode; //时间轮
};

//时间轮定时器
class CDTimerWheel : public CDRunable
{
public:
	CDTimerWheel();
	~CDTimerWheel();

	/* 开启定时器 */
	bool StartTimer();
	/* 关闭定时器 */
	void StopTimer();
	/* 添加定时器 */
	void* AddTimer(int,int,int,void*,TimerProc);
	/* 删除定时器 */
	void RemoveTimer(void*);

private:
	/* 重置定时器 */
	void ResetTimer(pWheelNode pNode, DWORD expires);
	/* 插入定时器 */
	void InsertTimer(pWheelNode pNode, CDWheel* pWheel, int nStep);
	/* 清理定时器 */
	void RemoveAll();
	void RemoveWheel(CDWheel* pWheel);
	/* 时间轮调度 */
	virtual bool Run();
	/* 获取下一个刻度时间槽的定时器 */
	pWheelNode Tick();
	/* 定时器节点链表操作 */
	static void LinkList(pWheelNode pTarget, pWheelNode pInsert);
	static void UnlinkList(pWheelNode pNode);

private:
	DWORD m_dwTime;
	CDLock m_lock;
	CDWheel* m_pWheel1;
	CDWheel* m_pWheel2;
};