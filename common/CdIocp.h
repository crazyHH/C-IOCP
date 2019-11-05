#pragma once

//head files
#include <list>
#include <hash_set>
#include <map>

#include "Define.h"
#ifndef IOCP_MFCDLLSERVER
//#include "CDIpFilter.h"
#endif //IOCP_MFCDLLSERVER
#include "../Common/CDLock.h"
#include "CDTimer.h"
//#include "CDRWLock.h"
#include "CDPacket.h"
#include "object_pool.hpp"
#include  <tchar.h>

#pragma comment(lib,"ws2_32.lib")

class CDHandler;
class CDIpFilter;

//Iocp message type
enum NetIoMessage
{
	IoInit = 1,
	IoRead,
	IoReadCompleted,
	IoZeroRead,
	IoZeroReadCompleted,
	IoWrite,
	IoWriteCompleted,
};

//max queued pending of connections
#define MAX_PENDING_CONNECTION	20		
//number of senquence number
#define SEND_SENQUENCE_NUMBER	41960	
//per cpu core thread
#define THREAD_NUMBER			2		
//max recv buffer size
#define MAX_RECV_BUFFER			16*1024
//max send buffer size
#define MAX_IOCP_BUFFER_SIZE	8*1024
//max pending send task
#define MAX_PENDING_SEND		5
//max pending recv task
#define MAX_PENDING_RECV		1

class CDIocpBuffer
{
public:
	CDIocpBuffer(UINT nAllocateSize=0);
	~CDIocpBuffer();

	//Memory operation.
	BOOL		AllocateBuffer(UINT nAllocateSize=0);
	BOOL		ReAllocateBuffer(UINT nAllocateSize=0);
	VOID		DeAllocateBuffer();
	VOID		ClearBuffer();

	inline BYTE	GetIoType()
	{
		return m_bIoType;
	};

	inline LPBYTE GetBuffer()
	{
		return m_bBuffer;
	};

	inline UINT	GetUsed()
	{
		return m_nUsed;
	};

	inline VOID	SetUsed(UINT nUsed)
	{
		m_nUsed = nUsed;
	};

	inline UINT	GetSize()
	{
		return m_nSize;
	};

	inline HANDLE GetHandle()
	{
		return m_hOperation;
	};

	inline VOID	SetOperation(BYTE bType)
	{
		m_bIoType = bType;
	};

	inline VOID	SetupWrite()
	{
		m_stWsaBuf.buf = reinterpret_cast<char*>(m_bBuffer);
		m_stWsaBuf.len = m_nUsed;
	};

	inline VOID	SetupWrite(HANDLE hSend, LPBYTE bBuffer, DWORD dwSize)
	{
		//
		// 修改日期: 2015/6/29
		// 错误的覆盖了缓冲区指针,导致内存泄露,以及错误的数据读写
		//
		m_hOperation = hSend;
		m_stWsaBuf.buf = reinterpret_cast<char*>(bBuffer);
		m_stWsaBuf.len = dwSize;
		m_nUsed = dwSize;
	};

	inline VOID	SetupRead()
	{
		m_stWsaBuf.buf = reinterpret_cast<char*>(m_bBuffer);
		m_stWsaBuf.len = m_nSize;
	};

	inline VOID	SetupZeroRead()
	{
		m_stWsaBuf.buf = NULL;
		m_stWsaBuf.len = 0;
	};

	//Buffer operation.
	BOOL AppendBuffer(LPBYTE lpWrite, UINT nSize);
	BOOL WriteBuffer(LPBYTE lpWrite, UINT nSize);

	VOID FlushBuffer(UINT nSize);
	CDIocpBuffer* SplitBuffer(UINT nSize);

	friend class CDIocp;

private:

	BYTE		m_bIoType;
	UINT		m_nUsed;
	UINT		m_nSize;
	LPBYTE		m_bBuffer;
	HANDLE		m_hOperation;
	WSABUF		m_stWsaBuf;
	OVERLAPPED	m_stOverLapped;
	DWORD		m_dwSerial;
};

//IocpBuffer map
typedef std::map<UINT, CDIocpBuffer*> BufferMap;

struct ClientContext
{
	SOCKET				m_Socket;
	unsigned			m_ID;
	CDLock				m_LockContext;
	volatile LONG		m_IoPending;

	//serial of all send buffer
	unsigned			m_SendSerial;
	//serial of current sent buffer
	unsigned			m_SentSerial;
	//buffer map wait to be send.
	BufferMap			m_SendMap;

	//serial of all recv buffer
	unsigned			m_RecvSerial;
	//serial of current recv buffer
	unsigned			m_RcvdSerial;
	//buffer list wait to be posted
	BufferMap			m_RecvMap;

	//Timer Handle
	void*				m_pTimer;
	//Bind to upper
	void*				m_Binding;
	//Net event handler.
	CDHandler*			m_pHandler;
	//Recv data caching.
	CDPacket			m_Packet;
};

//ClientContext map
typedef std::map<UINT, ClientContext*> ContextMap;

class CDIocp
{
public:
	CDIocp(CDHandler* pHandler, unsigned nMaxConn, unsigned nMaxFreeContext, unsigned nMaxFreeBuff);
	virtual ~CDIocp();

	BOOL	StartServer(unsigned nPort, unsigned nIoWorker);
	VOID	StopServer();
	BOOL	IsServerRun();

	//Bind socket to comletion port
	BOOL	BindToClientContext(SOCKET sock, HANDLE hBind=NULL, LPBYTE lpSend=NULL, DWORD dwSend=0);
	//Connect used for connect to a proxy server.
	BOOL	Connect(char* ip, int nPort, HANDLE hBind=NULL, LPBYTE lpSend=NULL, DWORD dwSend=0);
	//Disconnect a socket.
	BOOL	DisConnectClient(int nID);
	BOOL	Send(int nID, HANDLE hSend, LPBYTE bSendBuff, DWORD dwSendSize);
	VOID	SetHandler(ClientContext* pContext, CDHandler* pHandler);

#ifndef IOCP_MFCDLLSERVER
	//Interface for ip filter
	VOID	AddIntoFilter(sockaddr_in* pAddr);
	BOOL	IsInsideFilter(sockaddr_in* pAddr);
	VOID	RemoveFromFilter(sockaddr_in* pAddr);
	int		GetBanIp(LPBYTE bData, int nSize);
	VOID	SaveBanIp();
#endif // IOCP_MFCDLLSERVER

	//Interface for timer
	void	SetTimer(int nID, int nMillisecond, bool reset);
	void	KillTimer(int nID);

	//Find ClientContext by id;
	ClientContext* FindClientContext(int nID);

private:

	//CompletionPort operations
	BOOL	CreateCompletionPort();
	VOID	ClearCompletionPort();
	BOOL	BindToCompletionPort(SOCKET sock,ClientContext* pContext);

	inline BOOL	PostIoMessage(ClientContext* pContext, CDIocpBuffer* pBuffer, BYTE bType);
	inline BOOL	PostIoMessage(ClientContext* pContext);
	inline BOOL PostExitMessage();

	//Start working threads
	BOOL	StartListenThread();
	BOOL	StartIoWorkThread(UINT nWorker);

	//Shut down working threads
	VOID	ShutDownListenThread();
	VOID	ShutDownIoWorkThread();

	//Connection closed or not
	BOOL	IsActive(ClientContext* pContext);

	//Do connect here.
	BOOL	DisConnectClient(ClientContext* pContext);
	BOOL	DisConnectSocket(SOCKET sock);
	VOID	DisConnectAll();

	//ClientContext manager
	ClientContext* AllocateNewContext();
	VOID	ReleaseClientContext(ClientContext* pContext);
	VOID	ClearClientContext(ClientContext* pContext);
	BOOL	AddClientContext(ClientContext* pContext);
	VOID	RemoveClientContext(ClientContext* pContext);

	//CDIocpBuffer manager
	CDIocpBuffer* AllocateNewBuffer(UINT nAllocSize=MAX_IOCP_BUFFER_SIZE);
	VOID	ClearIocpBuffer(CDIocpBuffer* pBuffer);

	//Called by NetIoWorkThreads, process network operation.
	inline VOID	ProcessIoMessage(ClientContext* pContext, CDIocpBuffer* pIocpBuffer, BYTE bType, DWORD dwIoSize);

	//Called while new connection.
	inline VOID	OnIoInit(ClientContext* pContext, CDIocpBuffer* pIocpBuffer);
	//Post WSARecv operation.
	inline VOID	OnIoRead(ClientContext* pContext, CDIocpBuffer* pIocpBuffer);
	//WSARecv operation completed.
	inline VOID	OnIoReadCompleted(ClientContext* pContext, CDIocpBuffer* pIocpBuffer, UINT nRecv);
	//WSARecv operation with zero byte buffers.
	inline VOID	OnIoZeroRead(ClientContext* pContext, CDIocpBuffer* pIocpBuffer);
	//Indicate data recved with socket.
	inline VOID	OnIoZeroReadCompleted(ClientContext* pContext, CDIocpBuffer* pIocpBuffer);
	//Post WSASend operation.
	inline VOID	OnIoWrite(ClientContext* pContext, CDIocpBuffer* pIocpBuffer);
	//WSASend operation completed,Send buffer only copy to socket send buffer.
	inline VOID	OnIoWriteCompleted(ClientContext* pContext, CDIocpBuffer* pIocpBuffer, UINT nSend);

	//Number of pending IO on ClientContext, used for real release ClientContex.
	inline VOID	IncreasePendingIO(ClientContext* pContext);
	inline int	DecreasePendingIO(ClientContext* pContext);

	//
	//Notice: First i use a BufferList to restore pending send buffer,
	//but then send buffers get a lot of mistakes. After i choose a BufferMap,
	//send buffer function works well.
	//Solved: Call WSASend function must be synchronized.Call next WSASend
	// at WSASendCompleted callback will be fine...
	//

	inline VOID IncreaseSendSequence(ClientContext* pContext)
	{
		pContext->m_LockContext.Lock();
		pContext->m_SentSerial = (++pContext->m_SentSerial)%SEND_SENQUENCE_NUMBER;
		pContext->m_LockContext.Unlock();
	}
	inline VOID IncreaseRecvSequence(ClientContext* pContext)
	{
		pContext->m_LockContext.Lock();
		pContext->m_RcvdSerial = (++pContext->m_RcvdSerial)%SEND_SENQUENCE_NUMBER;
		pContext->m_LockContext.Unlock();
	}
	//Make send in order.
	inline VOID MakeSendInOrder(ClientContext* pContext, CDIocpBuffer* pIocpBuffer);
	inline CDIocpBuffer* GetNextSendBuffer(ClientContext* pContext, CDIocpBuffer* pIocpBuffer);

	//Make recv in order.
	inline VOID MakeRecvInOrder(ClientContext* pContext, CDIocpBuffer* pIocpBuffer);
	inline CDIocpBuffer* GetNextRecvBuffer(ClientContext* pContext, CDIocpBuffer* pIocpBuffer);

	//Operations for send/recv data.
	BOOL	OnSend(ClientContext* pContext, HANDLE hSend, LPBYTE bSendBuff, DWORD dwSendSize);
	BOOL	OnSend(ClientContext* pContext, CDIocpBuffer* pIocpBuffer);
	BOOL	OnRecv(ClientContext* pContext, CDIocpBuffer* pIocpBuffer);
	BOOL	OnZeroRecv(ClientContext* pContext, CDIocpBuffer* pIocpBuffer=NULL);

	//Enable TCPFlooding protection.
	BOOL	XPNTSYNFloodProtection(	int iValue=0,
									int iTcpMaxHalfOpen=100, 
									int iTcpMaxHalfOpenRetried=80, 
									int iTcpMaxPortsExhausted=5,
									int iTcpMaxConnectResponseRetransmissions=3
									);

	//Secure policy for Ban ip list.
	static int CALLBACK ConnectAcceptCondition(	LPWSABUF	lpCallerId,
												LPWSABUF	lpCallerData,
												LPQOS		lpSQOS,
												LPQOS		lpGQOS,
												LPWSABUF	lpCalleeId, 
												LPWSABUF	lpCalleeData,
												GROUP 		*g,
												DWORD		dwCallbackData
												);

/**	
 *	Write error log
 *	Error String: lpszLog
 *  Error Code: dwError
 */
	static BOOL	WriteErrorLog(LPCTSTR lpszLog, DWORD dwError);

private:

	static DWORD WINAPI ThreadIoWorkProc(LPVOID lPvoid);
	static DWORD WINAPI ThreadListenProc(LPVOID lPvoid);

	static void CALLBACK MyTimerProc(int, void*);

protected:

	BOOL			m_bInit;
	int				m_iWSAInitResult;

	//Number for every incoming client id
	unsigned		m_nID;

	//Iocp working config variables
	unsigned		m_nListenPort;
	unsigned		m_nConnection;
	unsigned		m_nMaxConnection;
	unsigned		m_nCreateIoWork;

	volatile LONG	m_bServerRun;
	volatile LONG	m_nRunIoWork;

	//Handles for Thread
	SOCKET			m_sockListen;
	HANDLE			m_hEventListen;
	HANDLE			m_hEventRun;
	HANDLE			m_hEventIoWork;
	HANDLE			m_hCompletionPort;

private:
	//Locks for ClientContext map.
	CDLock			m_LockContext;
	
	//Container for ClientContext
	ContextMap		m_MapContext;

	//Struct memory cache
	object_pool<CDIocpBuffer>	m_freeBuffer;
	object_pool<ClientContext>	m_freeContext;

#ifndef IOCP_MFCDLLSERVER
	//Ip filter
	//CDIpFilter		m_BanIp;
#endif // IOCP_MFCDLLSERVER
	
	//Timer for network event
	CDTimerWheel	m_Timer;

	//Internet call back
	CDHandler*		m_pHandler;
};