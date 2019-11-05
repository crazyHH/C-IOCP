#pragma once
#include "../Common/CDLock.h"
#include "CDRunable.h"

//��ʱ������
#define TIMER_TYPE_ONCE		0
#define TIMER_TYPE_RESET	1

//ÿ��ʱ���ֵ�������
#define WHEEL_COUNT_LEVEL1	1024
#define WHEEL_COUNT_LEVEL2	256

//ÿ�����Ӵ����ʱ�侫��(����)
#define WHEEL_PRECISION		50

typedef void (CALLBACK* TimerProc)(int, void*);

//��ʱ��ʱ��ڵ�
typedef struct WHEELNODE
{
	int			id;		// ��ʱ��id
	int			peroid;	// ʱ����
	int			expires;// ��ʱʱ��
	int			type;	// ��ʱ������
	int			del;	// �Ƿ�ɾ����ʱ��
	void*		param;	// �ص���������
	TimerProc	proc;	// �ص�����
	WHEELNODE*	prev;	// ������һ���ڵ�
	WHEELNODE*	next;	// ������һ���ڵ�
}WheelNode,*pWheelNode;

//ʱ����
struct CDWheel
{
	CDWheel(int num):m_nCrtPos(0),m_nWheelNum(num)
	{
		/* ��ʼ��ʱ���ָ��ڵ� */
		m_pWheelNode = new WheelNode[m_nWheelNum];
		ZeroMemory(m_pWheelNode, sizeof(WheelNode)*m_nWheelNum);
	};

	~CDWheel()
	{
		delete[] m_pWheelNode;
	};

	int	m_nCrtPos;	//ʱ���ֵ�ǰλ��
	int	m_nWheelNum;//ʱ���ֲ������
	pWheelNode m_pWheelNode; //ʱ����
};

//ʱ���ֶ�ʱ��
class CDTimerWheel : public CDRunable
{
public:
	CDTimerWheel();
	~CDTimerWheel();

	/* ������ʱ�� */
	bool StartTimer();
	/* �رն�ʱ�� */
	void StopTimer();
	/* ��Ӷ�ʱ�� */
	void* AddTimer(int,int,int,void*,TimerProc);
	/* ɾ����ʱ�� */
	void RemoveTimer(void*);

private:
	/* ���ö�ʱ�� */
	void ResetTimer(pWheelNode pNode, DWORD expires);
	/* ���붨ʱ�� */
	void InsertTimer(pWheelNode pNode, CDWheel* pWheel, int nStep);
	/* ����ʱ�� */
	void RemoveAll();
	void RemoveWheel(CDWheel* pWheel);
	/* ʱ���ֵ��� */
	virtual bool Run();
	/* ��ȡ��һ���̶�ʱ��۵Ķ�ʱ�� */
	pWheelNode Tick();
	/* ��ʱ���ڵ�������� */
	static void LinkList(pWheelNode pTarget, pWheelNode pInsert);
	static void UnlinkList(pWheelNode pNode);

private:
	DWORD m_dwTime;
	CDLock m_lock;
	CDWheel* m_pWheel1;
	CDWheel* m_pWheel2;
};