/**
*	模块: Define.h
*
*	功能: 定义公共接口/功能
*
*
*/
#pragma once

#include <winsock2.h>

/**
*	网络通讯协议定义
*
*/
#define DEFLISNUM   50
#define CHECK_CODE						0x1D		//协议包头标识
#define Z_OK							0			//压缩成功标志
#define	MAX_WRITE_RETRY					15			//重试写入文件次数

#define MAX_TASK_SIZE					1024*4		//最大请求任务数据长度

#define MAX_TIME_OUT					30			//最大超时时间
#define MAX_FUN_COUNT					64			//最大功能数
#define MAX_DEF_FUN_COUNT				16			//最大默认功能数

#define BYTE_TRUE						1			//单字节真
#define BYTE_FALSE						0			//单字节假
#define	MIN_COMPRESS_SIZE				260			//最小压缩长度
#define MIN_COMPRESS_SPAN_SIZE			1024*1024	//最小分卷压缩包大小


#define FLAG_RESEND						0x01		//resend
#define FLAG_FILE						0x02		//file
#define FLAG_PASSWORD					0x04		//password
#define FLAG_CLIENT						0x08		//client type
#define FLAG_TRANSIT					0x10		//transit data
#define	FLAG_COMPRESS					0x20		//compress data
#define FLAG_ENCRYPT					0x40		//encrpt data
#define FALG_PROTOCOL					0x80		//wrap package with protol

#define SET_HEADER_RESEND(bSet)			(bSet |= FLAG_RESEND)
#define SET_HEADER_FILE(bSet)			(bSet |= FLAG_FILE)
#define SET_HEADER_PASSWORD(bSet)		(bSet |= FLAG_PASSWORD)
#define SET_HEADER_CLIENT(bSet)			(bSet |= FLAG_CLIENT)
#define SET_HEADER_TRANSIT(bSet)		(bSet |= FLAG_TRANSIT)
#define SET_HEADER_COMPRESS(bSet)		(bSet |= FLAG_COMPRESS)
#define	SET_HEADER_ENCRYPT(bSet)		(bSet |= FLAG_ENCRYPT)

#define RESET_HEADER_RESEND(bSet)		(bSet &= ~FLAG_RESEND)
#define RESET_HEADER_FILE(bSet)			(bSet &= ~FLAG_FILE)
#define RESET_HEADER_PASSWORD(bSet)		(bSet &= ~FLAG_PASSWORD)
#define RESET_HEADER_CLIENT(bSet)		(bSet &= ~FLAG_CLIENT)
#define RESET_HEADER_TRANSIT(bSet)		(bSet &= ~FLAG_TRANSIT)
#define RESET_HEADER_COMPRESS(bSet)		(bSet &= ~FLAG_COMPRESS)
#define	RESET_HEADER_ENCRYPT(bSet)		(bSet &= ~FLAG_ENCRYPT)

#define IS_RESEND(bSet)					(FLAG_PASSWORD	== (bSet & FLAG_PASSWORD))
#define IS_FILE(bSet)					(FLAG_FILE		== (bSet & FLAG_FILE))
#define IS_PASSWORD(bSet)				(FLAG_PASSWORD	== (bSet & FLAG_PASSWORD))
#define IS_CLIENT(bSet)					(FLAG_CLIENT	== (bSet & FLAG_CLIENT))
#define IS_TRANSIT(bSet)				(FLAG_TRANSIT	== (bSet & FLAG_TRANSIT))
#define IS_COMPRESS(bSet)				(FLAG_COMPRESS	== (bSet & FLAG_COMPRESS))
#define	IS_ENCRYPT(bSet)				(FLAG_ENCRYPT	== (bSet & FLAG_ENCRYPT))


/* 回调函数定义 */
typedef BOOL(CALLBACK *SEND_DATAS)			(HANDLE, LPBYTE, DWORD, BYTE);
typedef BOOL(CALLBACK *RECV_DATA_CALLBACK)	(LPBYTE, DWORD, BYTE);
typedef BOOL(CALLBACK *FUN_INIT)			(HANDLE, LPBYTE, DWORD, LPBYTE, DWORD);
typedef BOOL(CALLBACK *FUN_RECV)			(HANDLE, LPBYTE, DWORD, BYTE);
typedef BOOL(CALLBACK *FUN_CLOSE)			(HANDLE, LPBYTE, DWORD, LPBYTE, DWORD);

enum RunServerType
{
	TCP_IOCP_SERVER = 1,
	TCP_CONNECT_SERVER = 2,
	UDP_SERVER = 3,
	SSL_SERVER = 4,
	HTTPS_SERVRE = 5
};

enum EncryptType
{
	XOREncrypt = 1,
};

enum CompressType
{
	zlibCompress = 1,
	NoneCompress,
};




#pragma pack(push,1)


//通讯协议包头
typedef struct SEND_PAGE_HEADER_ST
{
	BYTE		m_bCheckCode;			//一个字节标示符
	DWORD		m_wPageSize;			//数据包大小
	DWORD		m_wPageOrigSize;        //原始数据包大小
	BYTE		m_bSet;					//数据包配置
	BYTE        m_ProtocolType;         //协议类型
	BYTE        m_CompressType;         //压缩方式
	BYTE        m_EncryptType;          //加密方式
}SendPageHeaderSt, *pSendPageHeaderSt;

#define HDR_SIZE				sizeof(SendPageHeaderSt)	//包头大小

#define MAX_PACKET_BUFFER		20*1024						    //数据包最大长度
#define MAX_PACKET_SIZE			20*1024 - HDR_SIZE				//数据包最大可用长度
#define	MAX_SEND_SIZE			20*1024 - HDR_SIZE		        //最大发送数据包大小
#define MAX_RECV_SIZE			20*1024 - HDR_SIZE		        //最大接收数据包大小
#define MAX_SEND_FILE_SIZE	    4*1024		                    //最大发送文件数据包大小

//通迅指令包头
typedef struct PAGE_HEADER_ST
{
	BYTE		m_bFirstLayer;
	BYTE		m_bSecondLayer;
	BYTE		m_bThirdLayer;
	BYTE		m_bFourthLayer;
	//SOCKET		m_sock;
} PageHeaderSt, *pPageHeaderSt;

#define CMD_HDR_SIZE			sizeof(PageHeaderSt)


#pragma pack(pop)



/**
*	宏定义
*
*/
#define	safe_delete(p) \
if (p) \
{delete p; }

#define safe_close(h) \
if (INVALID_HANDLE_VALUE != h) \
{CloseHandle(h); }

#define safe_zero_delete(p) \
if (p) \
{delete p; p = NULL; }

#define safe_zero_close(h) \
if (INVALID_HANDLE_VALUE != h) \
{CloseHandle(h); h = INVALID_HANDLE_VALUE; }

#ifdef _DEBUG
#define	assert(r) if(!(r)) _asm int 3
#else
#define assert(r)
#endif //_DEBUG