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
 *	����:	Read
 *
 *	����:	�ӻ�������ȡ����
 *
 *	����ֵ:	ʵ�ʶ�ȡ���ݳ���
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
		//�����ȡ,һֱ������������
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
				//�պö�ȡλ���ڻ�����ĩ��
				nRet = nRead > m_nWrite ? m_nWrite : nRead;

				memcpy(bBuffer, m_bBuffer, nRet);

				m_nRead = nRet;
			}
			else
			{
				memcpy(bBuffer, &m_bBuffer[m_nRead], nUsedAfter);

				int nRest = nRead - nUsedAfter;

				//���ö�ָ��
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
 *	����:	Copy
 *
 *	����:	�ӻ�������������,���ô˺��������ƶ���ָ��
 *
 *	����ֵ:	ʵ�ʶ�ȡ���ݳ���
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
		//�����ȡ,һֱ������������
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
				//�պö�ȡλ���ڻ�����ĩ��
				nRet = nRead > m_nWrite ? m_nWrite : nRead;

				memcpy(bBuffer, m_bBuffer, nRet);
			}
			else
			{
				memcpy(bBuffer, &m_bBuffer[m_nRead], nUsedAfter);

				int nRest = nRead - nUsedAfter;

				//���ö�ָ��
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
 *	����:	Write
 *
 *	����:	д�뻺����,�������������,�Զ������㹻���ڴ���д��
 *
 *	����ֵ:	д�����ݳ���,����0��ʾʧ��
 *
 */
int	CDRingBuffer::Write(LPBYTE bBuffer, int nWrite)
{
	if(0 >= nWrite)
	{
		return 0;
	}

	//����������д��������㹻��
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

				//����дָ��
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
 *	����: Clear
 *
 *	����: ��ջ�����
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
 *	����: ReAllocate
 *
 *	����: ���仺�����ڴ�
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
		//����ԭ������������
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
 *	����: DeAllocate
 *
 *	����: ���������ڴ�
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
 *	����: GetUsed
 *
 *	����: ��ȡ��������д�볤��
 *
 */
int	CDRingBuffer::GetUsed()
{
	return m_nUsed;
}

/**
 *
 *	����: GetRest
 *
 *	����: ��ȡ��������д�볤��
 *
 */
int	CDRingBuffer::GetRest()
{
	return m_nSize - m_nUsed;
}

/**
 *
 *	����: GetSize
 *
 *	����: ��ȡ�������ܳ���
 *
 */
int	CDRingBuffer::GetSize()
{
	return m_nSize;
}

/**
 *
 *	����: GetBuffer
 *
 *	����: ��ȡ��������ָ��
 *
 */
LPBYTE	CDRingBuffer::GetBuffer()
{
	return m_bBuffer;
}

/**
 *
 *	����: IsEmpty
 *
 *	����: �������Ƿ�Ϊ��
 *
 */
bool CDRingBuffer::IsEmpty()
{
	return (0 == m_nUsed);
}