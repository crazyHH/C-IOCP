#include "stdafx.h"
#include "CDRingBuffer.h"

CDRingBuffer::CDRingBuffer()
:m_nRead(0)
,m_nWrite(0)
,m_nSize(0)
,m_nUsed(0)
,m_bBuffer(0)
{
}

CDRingBuffer::CDRingBuffer(int nAlloc)
:m_nRead(0)
,m_nWrite(0)
,m_nSize(0)
,m_nUsed(0)
,m_bBuffer(0)
{
	ReAllocate(nAlloc);
}

CDRingBuffer::~CDRingBuffer()
{
	DeAllocate();
}

/**
 *
 *	函数:	Read
 *
 *	功能:	从缓冲区读取数据
 *
 *	返回值:	实际读取数据长度
 *
 */
int	CDRingBuffer::Read(LPBYTE bBuffer, int nRead)
{
	if(0 >= nRead || !m_nUsed)
	{
		return 0;
	}

	int nRet = 0;

	if(m_nRead < m_nWrite)
	{
		//按需读取,一直读到缓冲区空
		int nUsed = m_nWrite - m_nRead;

		nRet = nRead > nUsed ? nUsed : nRead;

		memcpy(bBuffer, &m_bBuffer[m_nRead], nRet);

		m_nRead += nRet;
	}
	else/* if(m_nRead >= m_nWrite)*/
	{
		int nUsedAfter = m_nSize - m_nRead;

		if(nRead > nUsedAfter)
		{
			if(!nUsedAfter)
			{
				//刚好读取位置在缓冲区末端
				nRet = nRead > m_nWrite ? m_nWrite : nRead;

				memcpy(bBuffer, m_bBuffer, nRet);

				m_nRead = nRet;
			}
			else
			{
				memcpy(bBuffer, &m_bBuffer[m_nRead], nUsedAfter);

				int nRest = nRead - nUsedAfter;

				//重置读指针
				nRest = nRest > m_nWrite ? m_nWrite : nRest;

				memcpy(&bBuffer[nUsedAfter], m_bBuffer, nRest);

				nRet = nUsedAfter + nRest;

				m_nRead = nRest;
			}
		}
		else/*(nRead <= nUsedAfter)*/
		{
			nRet = nRead;

			memcpy(bBuffer, &m_bBuffer[m_nRead], nRet);

			m_nRead += nRet;
		}
	}

	m_nUsed -= nRet;

	return nRet;
}

/**
 *
 *	函数:	Copy
 *
 *	功能:	从缓冲区拷贝数据,调用此函数不会移动读指针
 *
 *	返回值:	实际读取数据长度
 *
 */
int	CDRingBuffer::Copy(LPBYTE bBuffer, int nRead)
{
	if(0 >= nRead || !m_nUsed)
	{
		return 0;
	}

	int nRet = 0;

	if(m_nRead < m_nWrite)
	{
		//按需读取,一直读到缓冲区空
		int nUsed = m_nWrite - m_nRead;

		nRet = nRead > nUsed ? nUsed : nRead;

		memcpy(bBuffer, &m_bBuffer[m_nRead], nRet);
	}
	else/* if(m_nRead >= m_nWrite)*/
	{
		int nUsedAfter = m_nSize - m_nRead;

		if(nRead > nUsedAfter)
		{
			if(!nUsedAfter)
			{
				//刚好读取位置在缓冲区末端
				nRet = nRead > m_nWrite ? m_nWrite : nRead;

				memcpy(bBuffer, m_bBuffer, nRet);
			}
			else
			{
				memcpy(bBuffer, &m_bBuffer[m_nRead], nUsedAfter);

				int nRest = nRead - nUsedAfter;

				//重置读指针
				nRest = nRest > m_nWrite ? m_nWrite : nRest;

				memcpy(&bBuffer[nUsedAfter], m_bBuffer, nRest);

				nRet = nUsedAfter + nRest;
			}
		}
		else/*(nRead <= nUsedAfter)*/
		{
			nRet = nRead;

			memcpy(bBuffer, &m_bBuffer[m_nRead], nRet);
		}
	}

	return nRet;
}

/**
 *
 *	函数:	Write
 *
 *	功能:	写入缓冲区,如果缓冲区已满,自动分配足够大内存再写入
 *
 *	返回值:	写入数据长度,返回0表示失败
 *
 */
int	CDRingBuffer::Write(LPBYTE bBuffer, int nWrite)
{
	if(0 >= nWrite)
	{
		return 0;
	}

	//缓冲区不足写入则分配足够大
	if(GetRest() < nWrite)
	{
		if(!ReAllocate(nWrite + m_nUsed))
		{
			return 0;
		}
	}

	if(m_nRead <= m_nWrite)
	{
		int nRestAfter = m_nSize - m_nWrite;

		if(nRestAfter > nWrite)
		{
			memcpy(&m_bBuffer[m_nWrite], bBuffer, nWrite);

			m_nWrite += nWrite;
		}
		else
		{
			if(!nRestAfter)
			{
				memcpy(m_bBuffer, bBuffer, nWrite);

				m_nWrite = nWrite;
			}
			else
			{
				memcpy(&m_bBuffer[m_nWrite], bBuffer, nRestAfter);

				//重置写指针
				int nRest = nWrite - nRestAfter;

				memcpy(m_bBuffer, &bBuffer[nRestAfter], nRest);

				m_nWrite = nRest;
			}
		}
	}
	else /*if(m_nRead > m_nWrite)*/
	{
		memcpy(&m_bBuffer[m_nWrite], bBuffer, nWrite);

		m_nWrite += nWrite;
	}

	m_nUsed += nWrite;

	return nWrite;
}

/**
 *
 *	函数: Clear
 *
 *	功能: 清空缓冲区
 *
 */
void CDRingBuffer::Clear()
{
	m_nRead		= 0;
	m_nWrite	= 0;
	m_nUsed		= 0;
}

/**
 *
 *	函数: ReAllocate
 *
 *	功能: 分配缓冲区内存
 *
 */
bool CDRingBuffer::ReAllocate(int nAlloc)
{
	if (nAlloc <= m_nSize)
	{
		return false;
	}

	LPBYTE bBuffer = (LPBYTE)malloc(nAlloc);

	if (!bBuffer)
	{
		return false;
	}

	int nUsed = 0;

	if (m_bBuffer)
	{
		//拷贝原来缓冲区数据
		nUsed = GetUsed();
		Read(bBuffer, nUsed);
		free(m_bBuffer);
	}

	m_bBuffer	= bBuffer;
	m_nSize		= nAlloc;
	m_nRead		= 0;
	m_nUsed		= nUsed;
	m_nWrite	= m_nUsed;

	return true;
}

/**
 *
 *	函数: DeAllocate
 *
 *	功能: 清理缓冲区内存
 *
 */
void CDRingBuffer::DeAllocate()
{
	if (m_bBuffer)
	{
		free(m_bBuffer);

		m_nRead		= 0;
		m_nWrite	= 0;
		m_nSize		= 0;
		m_nUsed		= 0;
		m_bBuffer	= NULL;
	}
}

/**
 *
 *	函数: GetUsed
 *
 *	功能: 获取缓冲区已写入长度
 *
 */
int	CDRingBuffer::GetUsed()
{
	return m_nUsed;
}

/**
 *
 *	函数: GetRest
 *
 *	功能: 获取缓冲区可写入长度
 *
 */
int	CDRingBuffer::GetRest()
{
	return m_nSize - m_nUsed;
}

/**
 *
 *	函数: GetSize
 *
 *	功能: 获取缓冲区总长度
 *
 */
int	CDRingBuffer::GetSize()
{
	return m_nSize;
}

/**
 *
 *	函数: GetBuffer
 *
 *	功能: 获取缓冲区基指针
 *
 */
LPBYTE	CDRingBuffer::GetBuffer()
{
	return m_bBuffer;
}

/**
 *
 *	函数: IsEmpty
 *
 *	功能: 缓冲区是否为空
 *
 */
bool CDRingBuffer::IsEmpty()
{
	return (0 == m_nUsed);
}