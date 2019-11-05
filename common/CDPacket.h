#pragma once

/**
 *	类名:CDPacket
 *
 *	概述:协议数据包，用于接收/发送完整数据包，完成数据包的处理与封装。
 *
 */
#include "CDRingBuffer.h"
#include "object_pool.hpp"

class CDPacket : public CDRingBuffer
{
public:
	CDPacket();
	virtual ~CDPacket();

	/* 数据包操作 */
	LPBYTE GetPacketBuffer();
	LPBYTE GetPacketHeaderBuffer();
	UINT GetPacketSize();

	VOID AppendHead();

	VOID BuildPacket(DWORD wPageOrigSize,BYTE bSet=0);
	VOID ParsePacket();
	
	CDPacket* SplitPacket();

	int	 IsPacketCompleted();
	UINT PacketSize();

	/* 缓冲区操作 */
	BOOL AppendBuffer(LPBYTE pBuffer, UINT nSize);
	UINT ReadBuffer(LPBYTE pBuffer, UINT nSize);
	CDPacket* SplitBuffer(UINT nSize);
	VOID ClearBuffer();

	/* 数据处理 */
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
