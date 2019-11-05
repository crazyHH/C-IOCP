#pragma once

/**
 *	����:CDPacket
 *
 *	����:Э�����ݰ������ڽ���/�����������ݰ���������ݰ��Ĵ������װ��
 *
 */
#include "CDRingBuffer.h"
#include "object_pool.hpp"

class CDPacket : public CDRingBuffer
{
public:
	CDPacket();
	virtual ~CDPacket();

	/* ���ݰ����� */
	LPBYTE GetPacketBuffer();
	LPBYTE GetPacketHeaderBuffer();
	UINT GetPacketSize();

	VOID AppendHead();

	VOID BuildPacket(DWORD wPageOrigSize,BYTE bSet=0);
	VOID ParsePacket();
	
	CDPacket* SplitPacket();

	int	 IsPacketCompleted();
	UINT PacketSize();

	/* ���������� */
	BOOL AppendBuffer(LPBYTE pBuffer, UINT nSize);
	UINT ReadBuffer(LPBYTE pBuffer, UINT nSize);
	CDPacket* SplitBuffer(UINT nSize);
	VOID ClearBuffer();

	/* ���ݴ��� */
	VOID Encrypt();
	VOID Decrypt();
	/*DEC*/
	VOID D_DEC();
	VOID E_DEC();

	BOOL Compress();
	BOOL Uncompress(DWORD wPageOrigSize);

	enum PacketState
	{
		PACKET_INVALID = -1,
		PACKET_INCOMPLETED = 0,
		PACKET_COMPLETED = 1
	};

	static CDPacket* AllocatePkt();
	static VOID CompletePkt(CDPacket* packet);

private:
	static object_pool<CDPacket> pool_packet;
};
