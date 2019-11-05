/**
 *	模块: CDRingBuffer
 *
 *	概述: 环形缓冲区,适用于网络通讯中的粘包拆包问题.
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

	/* 缓冲区读写操作 */
	int		Read(LPBYTE bBuffer, int nRead);
	int		Copy(LPBYTE bBuffer, int nRead);
	int		Write(LPBYTE bBuffer, int nWrite);
	void	Clear();

	/* 缓冲区内存管理 */
	bool	ReAllocate(int nAlloc);
	void	DeAllocate();

	int		GetUsed();
	int		GetRest();
	int		GetSize();
	LPBYTE	GetBuffer();
	bool	IsEmpty();

protected:
	/* 读取位置 */
	int		m_nRead;

	/* 写入位置 */
	int		m_nWrite;

	/* 缓冲区数据长度 */
	int		m_nUsed;

	/* 缓冲区长度 */
	int		m_nSize;

	/* 缓冲区指针 */
	LPBYTE	m_bBuffer;
};

#endif // _RING_BUFFER_H
