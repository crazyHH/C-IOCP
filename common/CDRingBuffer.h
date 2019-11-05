/**
 *	ģ��: CDRingBuffer
 *
 *	����: ���λ�����,����������ͨѶ�е�ճ���������.
 *
 */
#ifndef _RING_BUFFER_H
#define _RING_BUFFER_H

#include "Define.h"

class CDRingBuffer
{
public:
	CDRingBuffer();
	CDRingBuffer(int nAlloc);

	virtual ~CDRingBuffer();

	/* ��������д���� */
	int		Read(LPBYTE bBuffer, int nRead);
	int		Copy(LPBYTE bBuffer, int nRead);
	int		Write(LPBYTE bBuffer, int nWrite);
	void	Clear();

	/* �������ڴ���� */
	bool	ReAllocate(int nAlloc);
	void	DeAllocate();

	int		GetUsed();
	int		GetRest();
	int		GetSize();
	LPBYTE	GetBuffer();
	bool	IsEmpty();

protected:
	/* ��ȡλ�� */
	int		m_nRead;

	/* д��λ�� */
	int		m_nWrite;

	/* ���������ݳ��� */
	int		m_nUsed;

	/* ���������� */
	int		m_nSize;

	/* ������ָ�� */
	LPBYTE	m_bBuffer;
};

#endif // _RING_BUFFER_H
