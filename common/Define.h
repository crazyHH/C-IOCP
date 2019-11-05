/**
*	ģ��: Define.h
*
*	����: ���幫���ӿ�/����
*
*
*/
#pragma once

#include <winsock2.h>

/**
*	����ͨѶЭ�鶨��
*
*/
#define DEFLISNUM   50
#define CHECK_CODE						0x1D		//Э���ͷ��ʶ
#define Z_OK							0			//ѹ���ɹ���־
#define	MAX_WRITE_RETRY					15			//����д���ļ�����

#define MAX_TASK_SIZE					1024*4		//��������������ݳ���

#define MAX_TIME_OUT					30			//���ʱʱ��
#define MAX_FUN_COUNT					64			//�������
#define MAX_DEF_FUN_COUNT				16			//���Ĭ�Ϲ�����

#define BYTE_TRUE						1			//���ֽ���
#define BYTE_FALSE						0			//���ֽڼ�
#define	MIN_COMPRESS_SIZE				260			//��Сѹ������
#define MIN_COMPRESS_SPAN_SIZE			1024*1024	//��С�־�ѹ������С


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


/* �ص��������� */
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


//ͨѶЭ���ͷ
typedef struct SEND_PAGE_HEADER_ST
{
	BYTE		m_bCheckCode;			//һ���ֽڱ�ʾ��
	DWORD		m_wPageSize;			//���ݰ���С
	DWORD		m_wPageOrigSize;        //ԭʼ���ݰ���С
	BYTE		m_bSet;					//���ݰ�����
	BYTE        m_ProtocolType;         //Э������
	BYTE        m_CompressType;         //ѹ����ʽ
	BYTE        m_EncryptType;          //���ܷ�ʽ
}SendPageHeaderSt, *pSendPageHeaderSt;

#define HDR_SIZE				sizeof(SendPageHeaderSt)	//��ͷ��С

#define MAX_PACKET_BUFFER		20*1024						    //���ݰ���󳤶�
#define MAX_PACKET_SIZE			20*1024 - HDR_SIZE				//���ݰ������ó���
#define	MAX_SEND_SIZE			20*1024 - HDR_SIZE		        //��������ݰ���С
#define MAX_RECV_SIZE			20*1024 - HDR_SIZE		        //���������ݰ���С
#define MAX_SEND_FILE_SIZE	    4*1024		                    //������ļ����ݰ���С

//ͨѸָ���ͷ
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
*	�궨��
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