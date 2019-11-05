#include "StdAfx.h"
#include "CDPacket.h"
#include "Define.h"
//#include "DEC.h"
#ifndef IOCP_MFCDLLSERVER
#include "../../zlib/1.2.8/zlib.h"
	#ifdef _DEBUG
		#pragma comment(lib, "../zlib/1.2.8/zlibstat_D.lib")
	#else
		#pragma comment(lib, "../zlib/1.2.8/zlibstat.lib")
	#endif // _DEBUG

	object_pool<CDPacket> CDPacket::pool_packet(5000);
#else
	#include "../../zlib/1.2.3/zlib.h"
	#pragma comment(lib, "../zlib/1.2.3/zlibstat.lib")

	object_pool<CDPacket> CDPacket::pool_packet(100);
#endif //IOCP_MFCDLLSERVER

CDPacket::CDPacket()
:CDRingBuffer(MAX_PACKET_BUFFER)
{
}

CDPacket::~CDPacket()
{
}

VOID CDPacket::AppendHead()
{
	m_nWrite = m_nUsed = HDR_SIZE;
}

LPBYTE CDPacket::GetPacketBuffer()
{
	return &m_bBuffer[HDR_SIZE];
}

UINT CDPacket::GetPacketSize()
{
	return (m_nUsed - HDR_SIZE);
}

LPBYTE CDPacket::GetPacketHeaderBuffer()
{
	return &m_bBuffer[0];
}
/**
 *	����:	ParsePacket
 *
 *	����:	������ݰ��Ĳ��,��ѹ�������
 *
 *	����:	��
 */
VOID CDPacket::ParsePacket()
{
	pSendPageHeaderSt pSendHeader = (pSendPageHeaderSt)m_bBuffer;

	if (IS_ENCRYPT(pSendHeader->m_bSet))
	{
		Decrypt();
		//D_DEC();
	}

	if (IS_COMPRESS(pSendHeader->m_bSet))
	{
		Uncompress(pSendHeader->m_wPageOrigSize);
	}

	pSendHeader->m_wPageSize	= m_nUsed;
	pSendHeader->m_bSet			= 0;
}

/**
 *	����:	BuildPacket
 *
 *	����:	������ݰ��ķ�װ,ѹ�������
 *
 *	����:	bSet - ���ݰ�����
 */
VOID CDPacket::BuildPacket(DWORD wPageOrigSize , BYTE bSet)
{
	if (IS_COMPRESS(bSet))
	{
		if (!Compress()) 
		{
			RESET_HEADER_COMPRESS(bSet);
		}
	}

	if (IS_ENCRYPT(bSet))
	{
		Encrypt();
		//E_DEC();
	}

	pSendPageHeaderSt pSendHeader = (pSendPageHeaderSt)m_bBuffer;

	
	pSendHeader->m_bCheckCode	 = CHECK_CODE;
	pSendHeader->m_wPageSize	 = m_nUsed;
    pSendHeader->m_wPageOrigSize = wPageOrigSize + HDR_SIZE;
	pSendHeader->m_bSet			 = bSet;
	pSendHeader->m_ProtocolType  = TCP_IOCP_SERVER;
}

/**
 *	����:	SplitPacket
 *
 *	����:	�����ݻ����л�ȡ���������ݰ�
 *
 *	����:	��
 */
CDPacket* CDPacket::SplitPacket()
{
	int size = IsPacketCompleted();
	
	if(size > 0)
	{
		return SplitBuffer(size);
	}

	return (CDPacket*)size;
}

/**
 *	����:	IsPacketCompleted
 *
 *	����:	������ݰ��ĺϷ�����������
 *
 *	����ֵ:	PACKET_INCOMPLETED - ���ݰ�������
 *			PACKET_INVALID - ���ݰ����Ϸ�
 *			nSize - ���ݰ�����
 */
int CDPacket::IsPacketCompleted()
{
	if(m_nUsed < HDR_SIZE)
	{
		return PACKET_INCOMPLETED;
	}
	else
	{
		UINT nSize = PacketSize();

		if(nSize < HDR_SIZE || nSize > MAX_PACKET_BUFFER)
		{
			return PACKET_INVALID;
		}

		if(nSize > m_nUsed)
		{
			return PACKET_INCOMPLETED;
		}

		return nSize;
	}
}

/**
 *	����:	PacketSize
 *
 *	����:	���ݰ�ͷ�������ݰ�����
 *
 *	����:	��
 */
UINT CDPacket::PacketSize()
{
	UINT nSize = 0;

	if (m_nUsed >= HDR_SIZE)
	{
		BYTE bRead[HDR_SIZE+1];

		Copy(bRead, HDR_SIZE);


		nSize = ((pSendPageHeaderSt)bRead)->m_wPageSize;
	}

	return nSize;
}

BOOL CDPacket::AppendBuffer(LPBYTE pBuffer, UINT nSize)
{
	return Write(pBuffer, nSize);
}

UINT CDPacket::ReadBuffer(LPBYTE pBuffer, UINT nSize)
{
	return Read(pBuffer, nSize);
}

CDPacket* CDPacket::SplitBuffer(UINT nSize)
{
	if(nSize > m_nUsed)
	{
		nSize = m_nUsed;
	}

	CDPacket* pNewBuffer = CDPacket::AllocatePkt();

	if (pNewBuffer)
	{
		Read(pNewBuffer->m_bBuffer, nSize);

		pNewBuffer->m_nWrite	= nSize;
		pNewBuffer->m_nUsed		= nSize;
	}

	return pNewBuffer;
}

VOID CDPacket::ClearBuffer()
{
	Clear();
}

/**
 *	����:	Encrypt, Decrypt
 *
 *	����:	�����������
 *
 *	����:	��
 */
VOID CDPacket::Encrypt()
{
	DWORD dwEncrypt = m_nUsed - HDR_SIZE;
	LPDWORD lpEncrypt = (LPDWORD)&m_bBuffer[HDR_SIZE];

	DWORD dwSize4 = dwEncrypt/sizeof(DWORD);
	DWORD dwSize1 = dwEncrypt-dwSize4*sizeof(DWORD);

	for(int i = 0; i != dwSize4; i++)
		lpEncrypt[i] = ~lpEncrypt[i];

	for(int i = dwSize4*sizeof(DWORD)+HDR_SIZE; i != m_nUsed; i++)
		m_bBuffer[i] = ~m_bBuffer[i];

	//for(int i = HDR_SIZE; i != m_nUsed; i++)
	//	m_bBuffer[i] = ~m_bBuffer[i];
}

VOID CDPacket::Decrypt()
{
	DWORD dwEncrypt = m_nUsed - HDR_SIZE;
	LPDWORD lpEncrypt = (LPDWORD)&m_bBuffer[HDR_SIZE];

	DWORD dwSize4 = dwEncrypt/sizeof(DWORD);
	DWORD dwSize1 = dwEncrypt-dwSize4*sizeof(DWORD);

	for(int i = 0; i != dwSize4; i++)
		lpEncrypt[i] = ~lpEncrypt[i];

	for(int i = dwSize4*sizeof(DWORD)+HDR_SIZE; i != m_nUsed; i++)
		m_bBuffer[i] = ~m_bBuffer[i];

	//for(int i = HDR_SIZE; i != m_nUsed; i++)
	//	m_bBuffer[i] = ~m_bBuffer[i];
}

/**
 *	����:	Compress, Uncompress
 *
 *	����:	zlib��ѹ�����ѹ��
 *
 *	����:	��
 */
BOOL CDPacket::Compress()
{
	assert(m_nUsed);

	BOOL	bRet		= FALSE;
	ULONG	desLen		= compressBound(m_nUsed - HDR_SIZE);
	LPBYTE	pCompress	= (LPBYTE)malloc(desLen);

	if (!pCompress)
		{ return bRet; }

	if(Z_OK == compress(pCompress, &desLen, &m_bBuffer[HDR_SIZE], m_nUsed - HDR_SIZE))
	{
		memcpy(&m_bBuffer[HDR_SIZE], pCompress, desLen);

		m_nRead		= 0;
		m_nWrite	= desLen + HDR_SIZE;
		m_nUsed		= m_nWrite;

		bRet		= TRUE;
	}

	free(pCompress);
	return bRet;
}

BOOL CDPacket::Uncompress(DWORD wPageOrigSize)
{
	assert(m_nUsed);

	BOOL	bRet		= FALSE;
	ULONG	desLen		= wPageOrigSize - HDR_SIZE; //MAX_PACKET_SIZE;
	LPBYTE	pCompress	= (LPBYTE)malloc(desLen);

	if (!pCompress)
		{ return bRet; }

	if(Z_OK == uncompress(pCompress, &desLen, &m_bBuffer[HDR_SIZE], m_nUsed - HDR_SIZE))
	{
		memcpy(&m_bBuffer[HDR_SIZE], pCompress, desLen);

		m_nRead		= 0;
		m_nWrite	= desLen + HDR_SIZE;
		m_nUsed		= m_nWrite;

		bRet		= TRUE;
	}

	free(pCompress);
	return bRet;
}

CDPacket* CDPacket::AllocatePkt()
{
	// �������AllocPacket, �������ĳ��Ȳ���
	CDPacket* packet = pool_packet.construct();
	//CDPacket* packet = new CDPacket();
	if (packet) { 
		packet->AppendHead(); 
	}
	
	return packet;
}

VOID CDPacket::CompletePkt(CDPacket* packet)
{
	packet->Clear();
	pool_packet.destroy(packet);
	//delete packet;
}

//VOID CDPacket::D_DEC()
//{
//   // ��Կ����
//   char key[]={0,2,1,0,9,4,5,1};
//   char decrypt_text[MAX_PACKET_SIZE];
//   
//   DWORD dwEncrypt = m_nUsed - HDR_SIZE;
//   DWORD dwSize8 = dwEncrypt/8;
//   //DWORD dwSize1 = dwEncrypt-dwSize8*8;
//
//   //DES_Act(decrypt_text, (char*)&m_bBuffer[HDR_SIZE], dwSize8*8, key, sizeof(key), 1);
//   memcpy(&m_bBuffer[HDR_SIZE], decrypt_text, dwSize8*8);
//
//   for(int i = dwSize8*8+HDR_SIZE; i != m_nUsed; i++)
//		m_bBuffer[i] = ~m_bBuffer[i];
//}
//
//VOID CDPacket::E_DEC()
//{
//	// 8λ��Կ����
//	char key[]={0,2,1,0,9,4,5,1};
//	char encrypt_text[MAX_PACKET_SIZE];
//
//    DWORD dwEncrypt = m_nUsed - HDR_SIZE;
//	DWORD dwSize8 = dwEncrypt/8;
//	//DWORD dwSize1 = dwEncrypt-dwSize8*8;
//    
//	//����8���ֽ�DES����
//	//DES_Act(encrypt_text, (char*)&m_bBuffer[HDR_SIZE], dwSize8*8, key, sizeof(key), 0);
//	memcpy(&m_bBuffer[HDR_SIZE], encrypt_text, dwSize8*8);
//    
//	//ʣ�µ��ֽ�ȡ������
//	for(int i = dwSize8*8+HDR_SIZE; i != m_nUsed; i++)
//		m_bBuffer[i] = ~m_bBuffer[i];;
//}