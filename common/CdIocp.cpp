#include "stdafx.h"
#include "CDIocp.h"
#include "../Common/CDHandler.h"

//CDIocpBuffer
CDIocpBuffer::CDIocpBuffer(UINT nAllocateSize)
{
	m_bIoType	 = 0;
	m_nUsed		 = 0;
	m_nSize		 = 0;
	m_bBuffer	 = NULL;
	m_hOperation = NULL;

	ZeroMemory(&m_stWsaBuf, sizeof(WSABUF));
	ZeroMemory(&m_stOverLapped, sizeof(OVERLAPPED));

	AllocateBuffer(nAllocateSize);
}

CDIocpBuffer::~CDIocpBuffer()
{
	DeAllocateBuffer();
}

BOOL CDIocpBuffer::AllocateBuffer(UINT nAllocateSize)
{
	if (!nAllocateSize) {
		return TRUE;
	}

	m_bBuffer = (LPBYTE)malloc(nAllocateSize);

	if (m_bBuffer) {
		m_nUsed = 0;
		m_nSize = nAllocateSize;
		return TRUE;
	}

	return FALSE;
}

BOOL CDIocpBuffer::ReAllocateBuffer(UINT nAllocateSize)
{
	if (nAllocateSize > m_nSize) {
		m_bBuffer = (LPBYTE)realloc(m_bBuffer, nAllocateSize);
		if (m_bBuffer) {
			m_nSize = nAllocateSize;
			return TRUE;
		}
	}

	return FALSE;
}

VOID CDIocpBuffer::DeAllocateBuffer()
{
	if (m_bBuffer) {
		free(m_bBuffer);
		m_bBuffer = NULL;
		m_nUsed	= 0;
		m_nSize	= 0;
	}
}

VOID CDIocpBuffer::ClearBuffer()
{
	m_nUsed	= 0;
}

BOOL CDIocpBuffer::AppendBuffer(LPBYTE lpWrite, UINT nSize)
{
	if((!lpWrite) || (m_nUsed + nSize > m_nSize))
	{
		return FALSE;
	}

	memcpy_s(&m_bBuffer[m_nUsed], m_nSize - m_nUsed, lpWrite, nSize);
	m_nUsed += nSize;

	return FALSE;
}

BOOL CDIocpBuffer::WriteBuffer(LPBYTE lpWrite, UINT nSize)
{
	if((!lpWrite) || (nSize > m_nSize))
	{
		return FALSE;
	}

	memcpy_s(m_bBuffer, m_nSize, lpWrite, nSize);
	m_nUsed = nSize;

	return TRUE;
}

VOID CDIocpBuffer::FlushBuffer(UINT nSize)
{
	if (nSize >= m_nUsed)
	{
		return;
	}

	memmove(m_bBuffer, &m_bBuffer[nSize], m_nUsed - nSize);
	m_nUsed -= nSize;
}

CDIocpBuffer* CDIocpBuffer::SplitBuffer(UINT nSize)
{
	CDIocpBuffer* pNewBuffer = new CDIocpBuffer(nSize);

	if(nSize > m_nUsed || !pNewBuffer)
	{
		return pNewBuffer;
	}

	pNewBuffer->WriteBuffer(m_bBuffer, nSize);
	FlushBuffer(nSize);

	return pNewBuffer;
}

//CDIocp
CDIocp::CDIocp(CDHandler* pHandler, unsigned nMaxConn, unsigned nMaxFreeContext,unsigned nMaxFreeBuff)
:m_pHandler(pHandler)
,m_bInit(FALSE)
,m_bServerRun(FALSE)
,m_nID(0)
,m_nListenPort(0)
,m_nConnection(0)
,m_nMaxConnection(nMaxConn)
,m_nCreateIoWork(2)
,m_nRunIoWork(0)
,m_sockListen(INVALID_SOCKET)
,m_hEventListen(0)
,m_hEventRun(0)
,m_hEventIoWork(0)
,m_hCompletionPort(0)
,m_freeBuffer(nMaxFreeBuff)
,m_freeContext(nMaxFreeContext)
{
	WSADATA wsaData;
	m_iWSAInitResult = WSAStartup(MAKEWORD(2,2), &wsaData);
}

CDIocp::~CDIocp()
{
	StopServer();

	if (m_iWSAInitResult == NO_ERROR) { 
		WSACleanup(); 
	}
}

BOOL CDIocp::StartServer(unsigned nPort, unsigned nIoWorker)
{
	if(NO_ERROR != m_iWSAInitResult)
	{
		WriteErrorLog(	_T("[StartIocpServer]:WSAStartup failed."), 
						m_iWSAInitResult
						);
		return FALSE;
	}
	//Close iocp server if is running.
	if (m_bServerRun) { 
		StopServer(); 
	}

	BOOL bResult	= FALSE;

	m_nID			= 0;
	m_nListenPort	= nPort;
	m_nConnection	= 0;
	m_nRunIoWork	= 0;
	m_nCreateIoWork	= nIoWorker;

	do
	{
		::InterlockedIncrement(&this->m_bServerRun);

		//if (!m_LockContext.InitializeLock()) {
		//	break;
		//}

		if (!m_Timer.StartTimer()) { 
			break; 
		}

		if (!CreateCompletionPort()) { 
			break; 
		}

		if (!StartIoWorkThread(this->m_nRunIoWork)) { 
			break; 
		}

		if (!StartListenThread()) {
			if (this->m_hEventRun) { 
				SetEvent(this->m_hEventRun); 
			}
			break;
		}

		bResult = TRUE;

	}while(FALSE);

	if (!bResult) { 
		StopServer(); 
	}

	return bResult;
}

VOID CDIocp::StopServer()
{
	::InterlockedDecrement(&this->m_bServerRun);

	//Shut down listen thread first.
	ShutDownListenThread();

	//Disconnect all.
	DisConnectAll();

	//Release pending io.
	ShutDownIoWorkThread();

	ClearCompletionPort();

	//Close timer.
	m_Timer.Stop();

	//Clear share lock.
	//m_LockContext.DeleteLock();
}

BOOL CDIocp::IsServerRun()
{
	return m_bServerRun;
}

BOOL CDIocp::CreateCompletionPort()
{
	if (m_hCompletionPort)
	{
		return FALSE;
	}

	m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, m_nCreateIoWork);

	if (!m_hCompletionPort)
	{
		WriteErrorLog(	_T("[CreateCompletionPort]:CreateIoCompletionPort failed."),
						::GetLastError()
						);

		return FALSE;
	}

	return TRUE;
}

VOID CDIocp::ClearCompletionPort()
{
	if (m_hCompletionPort)
	{
		CloseHandle(m_hCompletionPort);
		m_hCompletionPort = NULL;
	}
}

BOOL CDIocp::BindToCompletionPort(SOCKET sock, ClientContext* pContext)
{
	HANDLE hPort = CreateIoCompletionPort(	(HANDLE)sock, 
											m_hCompletionPort, 
											(ULONG_PTR)pContext, 
											NULL
											);

	if(hPort != m_hCompletionPort)
	{
		DWORD dwError = ::GetLastError();
		WriteErrorLog(	_T("[BindToCompletionPort]:CreateIoCompletionPort failed."),
						dwError
						);
		return FALSE;
	}

	return TRUE;
}

BOOL CDIocp::BindToClientContext(SOCKET sock, HANDLE hBind, LPBYTE lpSend, DWORD dwSend)
{
	const char chOpt = 1;
	int nResult = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &chOpt, sizeof(char));
	/*
	BOOL bKeepAlive = TRUE;     
    nResult &= setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char*)&bKeepAlive, sizeof(bKeepAlive)); 
	//Set socket keepalive options.
	//But it seems doesn't work while binding on CompletionPort.
	tcp_keepalive inKeepAlive = {1, 3000, 5000};
	tcp_keepalive outKeepAlive = {0};
	DWORD dwRet = 0;
	
	nResult &= WSAIoctl(sock, 
						SIO_KEEPALIVE_VALS, 
						&inKeepAlive,
						sizeof(tcp_keepalive),
						&outKeepAlive,
						sizeof(tcp_keepalive),
						&dwRet,
						NULL,
						NULL
						);
	int nBufLen;
	int nOptLen = sizeof(int);
	//获取当前套接字的接受数据缓冲区大小
	nResult &= getsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char*)&nBufLen, &nOptLen);
	//增大套接字数据缓冲区大小,提高传输速度
	nBufLen << 3;
	nResult &= setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char*)&nBufLen, nOptLen);
	nResult &= setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char*)&nBufLen, nOptLen);
	*/

	if (SOCKET_ERROR == nResult)
	{
		DisConnectSocket(sock);

		WriteErrorLog(	_T("[BindToClientContext]:Set socket nagle failed."),
						WSAGetLastError()
						);
		return FALSE;
	}

	ClientContext* pContext = AllocateNewContext();

	if (!pContext)
	{
		DisConnectSocket(sock);

		WriteErrorLog(	_T("[BindToClientContext]:AllocateNewContext failed."),
						NULL
						);
		return FALSE;
	}

	pContext->m_Socket		= sock;
	pContext->m_ID          = ::InterlockedIncrement((volatile LONG*)&m_nID);
	pContext->m_IoPending	= 0;
	pContext->m_SentSerial	= 0;
	pContext->m_SendSerial	= 0;
	pContext->m_RecvSerial  = 0;
	pContext->m_RcvdSerial	= 0;
	pContext->m_pTimer		= NULL;
	pContext->m_Binding		= hBind;
	pContext->m_pHandler	= m_pHandler;

	// Modify at 2015/7/23 PM.
	// For proxy server first pending packet.
	pContext->m_Packet.AppendBuffer(lpSend, dwSend);

	if (!BindToCompletionPort(sock, pContext))
	{
		DisConnectSocket(sock);
		ClearClientContext(pContext);
		return FALSE;
	}

	return PostIoMessage(pContext, AllocateNewBuffer(), IoInit);
}

BOOL CDIocp::PostIoMessage(ClientContext* pContext, CDIocpBuffer* pBuffer, BYTE bType)
{
	//Add number of pending io.
	IncreasePendingIO(pContext);

	//Set operation type on IocpBuffer.
	pBuffer->SetOperation(bType);

	BOOL bResult = PostQueuedCompletionStatus(m_hCompletionPort, 0, (DWORD)pContext, &pBuffer->m_stOverLapped);

	if(!bResult && GetLastError() != ERROR_IO_PENDING)
	{
		ReleaseClientContext(pContext);
		ClearIocpBuffer(pBuffer);

		WriteErrorLog(	_T("[PostIoMessage]: PostQueuedCompletionStatus failed."),
						::GetLastError()
						);
	}

	return bResult;
}

BOOL CDIocp::PostIoMessage(ClientContext* pContext)
{
	//Add number of pending io.
	IncreasePendingIO(pContext);

	BOOL bResult = PostQueuedCompletionStatus(m_hCompletionPort, 0, (ULONG_PTR)pContext, NULL);

	if(!bResult && GetLastError() != ERROR_IO_PENDING)
	{
		ReleaseClientContext(pContext);

		WriteErrorLog(	_T("[PostIoMessage]: PostQueuedCompletionStatus failed."),
						::GetLastError()
						);
	}

	return bResult;
}

BOOL CDIocp::PostExitMessage()
{
	BOOL bResult = PostQueuedCompletionStatus(m_hCompletionPort, 0, NULL, NULL);

	if (!bResult && GetLastError( ) != ERROR_IO_PENDING)
	{
		WriteErrorLog(	_T("[PostIoMessage]: PostQueuedCompletionStatus failed."),
						::GetLastError()
						);
	}

	return bResult;
}

BOOL CDIocp::StartListenThread()
{
	if (-1 == m_nListenPort)
	{
		//Used for MFCDllServer based IocpServer.
		//No need listen procedure.
		return TRUE;
	}

	if (INVALID_SOCKET != this->m_sockListen)
	{
		closesocket(this->m_sockListen);
		this->m_sockListen = INVALID_SOCKET;
	}
#ifdef _IPV6
	this->m_sockListen = WSASocket(AF_INET6, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);
#else
	this->m_sockListen = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);
#endif //_IPV6

	if (INVALID_SOCKET == this->m_sockListen)
	{
		return FALSE;
	}

	//Create event for listen network event.
	if (!this->m_hEventListen)
	{
		this->m_hEventListen = ::WSACreateEvent();

		if (WSA_INVALID_EVENT == this->m_hEventListen)
		{
			WriteErrorLog(	_T("[StartListenThread]:WSACreateEvent failed."),
							::GetLastError()
							);
			return FALSE;
		}
	}
	//Create event for listen thread exit.
	if (!this->m_hEventRun)
	{
		this->m_hEventRun = ::CreateEvent(NULL, FALSE, FALSE, NULL);

		if (!this->m_hEventRun)
		{
			WriteErrorLog(	_T("[StartListenThread]:CreateEvent failed."),
							::GetLastError()
							);
			return FALSE;
		}
	}

	//Setup listen procedure.
	int nResult = WSAEventSelect(	m_sockListen,
									m_hEventListen,
									FD_ACCEPT
									);

	if (nResult == SOCKET_ERROR)
	{
		WriteErrorLog(	_T("[StartListenThread]:WSAEventSelect failed."),
						::WSAGetLastError()
						);
		return FALSE;
	}

	SOCKADDR_IN addrServer;

	addrServer.sin_port			= htons(m_nListenPort);
	addrServer.sin_family		= AF_INET;
	addrServer.sin_addr.s_addr	= INADDR_ANY;

	nResult = bind(m_sockListen, (LPSOCKADDR)&addrServer, sizeof(sockaddr));

	if (nResult == SOCKET_ERROR)
	{
		WriteErrorLog(	_T("[StartListenThread]:bind failed."),
						::WSAGetLastError()
						);
		return FALSE;
	}

	nResult = listen(m_sockListen, MAX_PENDING_CONNECTION);

	if (nResult == SOCKET_ERROR)
	{
		WriteErrorLog(	_T("[StartListenThread]:listen failed."),
						::WSAGetLastError()
						);
		return FALSE;
	}

	//Begin listen thread.
	HANDLE hThread = ::CreateThread(NULL, 
									NULL, 
									ThreadListenProc, 
									(LPVOID)this, 
									CREATE_SUSPENDED, 
									NULL
									);

	if (!hThread)
	{
		WriteErrorLog(	_T("[StartListenThread]:CreateThread failed."),
						::GetLastError()
						);
		return FALSE;
	}

	::ResumeThread(hThread);
	::CloseHandle(hThread);

	return TRUE;
}

BOOL CDIocp::StartIoWorkThread(UINT nWorker)
{
	if (!m_hEventIoWork)
	{
		m_hEventIoWork = ::CreateEvent(NULL, FALSE, FALSE, NULL);

		if (!this->m_hEventIoWork)
		{
			WriteErrorLog(	_T("[StartIoWorkThread]:CreateEvent failed."),
							::GetLastError()
							);
			return FALSE;
		}
	}

	HANDLE hThread = NULL;

	for (int i = 0; i != m_nCreateIoWork; i++)
	{
		hThread = ::CreateThread(NULL, NULL, ThreadIoWorkProc, (LPVOID)this, CREATE_SUSPENDED, NULL);

		if (!hThread)
		{
			WriteErrorLog(	_T("[StartIoWorkThread]:CreateThread failed."),
							::GetLastError()
							);
			return FALSE;
		}

		::InterlockedIncrement(&m_nRunIoWork);

		ResumeThread(hThread);
		CloseHandle(hThread);
	}

	return TRUE;
}

VOID CDIocp::ShutDownListenThread()
{
	//Wait entil thread exit.
	if (m_hEventRun)
	{
		WaitForSingleObject(this->m_hEventRun, INFINITE);

		CloseHandle(m_hEventRun);
		m_hEventRun = NULL;
	}

	//Clear listen procedure.
	if (INVALID_SOCKET != m_sockListen)
	{
		closesocket(m_sockListen);
		m_sockListen = INVALID_SOCKET;
	}

	if (m_hEventListen)
	{
		WSACloseEvent(m_hEventListen);
		m_hEventListen = NULL;
	}
}

VOID CDIocp::ShutDownIoWorkThread()
{
	if (m_hEventIoWork)
	{
		if (m_nRunIoWork)
		{
			PostExitMessage();

			WaitForSingleObject(m_hEventIoWork, INFINITE);
		}

		CloseHandle(m_hEventIoWork);
		m_hEventIoWork = NULL;
	}
}

BOOL CDIocp::Connect(char* ip, int nPort, HANDLE hBind, LPBYTE lpSend, DWORD dwSend)
{
	if (!this->m_bServerRun)
	{
		return FALSE;
	}

	SOCKET		sockClient;
	int			nResult = -1;

	hostent* pHostent = NULL;

	pHostent = gethostbyname(ip);
	if (pHostent == NULL)
		return false;

#ifdef _IPV6
	SOCKADDR_IN6 sockAddr;
	sockAddr.sin_family	= AF_INET6;
	sockAddr.sin6_port	= htons(nPort);
	sockAddr.sin_addr   = *((struct in_addr *)pHostent->h_addr);

	sockClient = WSASocket(AF_INET6, SOCK_STREAM, IPPROTO_IPV6, NULL, 0, WSA_FLAG_OVERLAPPED);
#else
	SOCKADDR_IN sockAddr;
	sockAddr.sin_family				= AF_INET;
	sockAddr.sin_port				= htons(nPort);
	sockAddr.sin_addr               = *((struct in_addr *)pHostent->h_addr);

	sockClient = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);
#endif //_IPV6

	if(INVALID_SOCKET == sockClient)
	{
		WriteErrorLog(	_T("[ConnectToServer]:Create socket failed."),
						::WSAGetLastError()
						);
		return FALSE;
	}

	//int time = 2000;
	//setsockopt(sockClient, SOL_SOCKET, SO_RCVTIMEO, (char *)&time, sizeof(time));

	nResult = ::connect(sockClient, (sockaddr*)&sockAddr, sizeof(sockAddr));

	if(SOCKET_ERROR == nResult)
	{
		WriteErrorLog(	_T("[ConnectToServer]:connect failed."),
						::WSAGetLastError()
						);
		return FALSE;
	}

	return BindToClientContext(sockClient, hBind, lpSend, dwSend);
}

BOOL CDIocp::IsActive(ClientContext* pContext)
{
	pContext->m_LockContext.Lock();
	BOOL bActive = (pContext->m_Socket != INVALID_SOCKET);
	pContext->m_LockContext.Unlock();

	return bActive;
}

BOOL CDIocp::DisConnectClient(int nID)
{
	ClientContext* pContext = FindClientContext(nID);

	if (!pContext) 
		{ return FALSE; }

	return PostIoMessage(pContext);
}

BOOL CDIocp::DisConnectClient(ClientContext* pContext)
{
	if (!pContext)
	{
		return FALSE;
	}

	pContext->m_LockContext.Lock();
	SOCKET sock = pContext->m_Socket;
	pContext->m_Socket = INVALID_SOCKET;
	pContext->m_LockContext.Unlock();

	if (INVALID_SOCKET != sock)
	{
		//Notify to upper.
		if (pContext->m_pHandler)
		{
			pContext->m_pHandler->OnClose(pContext);
		}

		//Remove from ClientContext map.
		RemoveClientContext(pContext);

		LINGER lingerStruct;
		lingerStruct.l_onoff = 1;
		lingerStruct.l_linger = 0;

		setsockopt( sock, 
					SOL_SOCKET, 
					SO_LINGER,
					(char *)&lingerStruct, 
					sizeof(lingerStruct) 
					);

		// Now close the socket handle.  This will do an abortive or graceful close, as requested.  
		CancelIo((HANDLE)sock);
		shutdown(sock, SD_BOTH);
		closesocket(sock);
	}

	return TRUE;
}

BOOL CDIocp::DisConnectSocket(SOCKET sock)
{
	if(INVALID_SOCKET == sock)
	{
		return FALSE;
	}

	LINGER lingerStruct;
	lingerStruct.l_onoff = 1;
	lingerStruct.l_linger = 0;

	setsockopt( sock, 
				SOL_SOCKET, 
				SO_LINGER,
				(char *)&lingerStruct, 
				sizeof(lingerStruct) 
				);

	// Now close the socket handle.  This will do an abortive or graceful close, as requested.  
	CancelIo((HANDLE)sock);
	shutdown(sock, SD_BOTH);
	closesocket(sock);

	return TRUE;
}

VOID CDIocp::DisConnectAll()
{
	ClientContext* pContext = NULL;

	ContextMap::iterator iter;
	
	while (!this->m_MapContext.empty())
	{
		iter = m_MapContext.begin();

		pContext = (*iter).second;

		DisConnectClient(pContext);
	}
}

VOID CDIocp::SetHandler(ClientContext* pContext, CDHandler* pHandler)
{
	InterlockedExchangePointer((void**)&pContext->m_pHandler, pHandler);
}

ClientContext* CDIocp::AllocateNewContext()
{
	ClientContext* pContext = m_freeContext.construct();

	if (pContext)
	{
		::InterlockedIncrement((volatile LONG*)&m_nConnection);
	}

	return pContext;
}

VOID CDIocp::ReleaseClientContext(ClientContext* pContext)
{
	int nIoPending = DecreasePendingIO(pContext);

	if(0 > nIoPending)
	{
		WriteErrorLog(	_T("[ReleaseClientContext]:Number of pending io less than 0."),
						nIoPending
						);
	}

	if (!nIoPending)
	{
		ClearClientContext(pContext);
	}
}

VOID CDIocp::ClearClientContext(ClientContext* pContext)
{
	if (!pContext)
	{
		return;
	}

	::InterlockedDecrement((volatile LONG*)&m_nConnection);

	//Clear Send Buffer map.
	BufferMap::iterator iter = pContext->m_SendMap.begin();
	for (iter; iter != pContext->m_SendMap.end(); ++iter)
	{
		ClearIocpBuffer((CDIocpBuffer*)(*iter).second);
	}
	pContext->m_SendMap.clear();

	//Clear Recv Buffer map.
	iter = pContext->m_RecvMap.begin();
	for (iter; iter != pContext->m_RecvMap.end(); ++iter)
	{
		ClearIocpBuffer((CDIocpBuffer*)(*iter).second);
	}
	pContext->m_RecvMap.clear();
	pContext->m_Packet.Clear();

	m_freeContext.destroy(pContext);
}

BOOL CDIocp::AddClientContext(ClientContext* pContext)
{
	m_LockContext.Lock();

	//Used socket for m_ID
	m_MapContext[pContext->m_ID] = pContext;

	m_LockContext.Unlock();

	return TRUE;
}

ClientContext* CDIocp::FindClientContext(int nID)
{
	ClientContext* pFind = NULL;

	m_LockContext.Lock();

	ContextMap::iterator iter = m_MapContext.find(nID);

	if (iter != m_MapContext.end())
	{
		pFind = (*iter).second;
	}

	m_LockContext.Unlock();

	return pFind;
}

VOID CDIocp::RemoveClientContext(ClientContext* pContext)
{
	m_LockContext.Lock();

	ContextMap::iterator iter = m_MapContext.find(pContext->m_ID);

	if (iter != m_MapContext.end())
	{
		m_MapContext.erase(iter);
	}

	m_LockContext.Unlock();
}

CDIocpBuffer* CDIocp::AllocateNewBuffer(UINT nAllocSize)
{
	CDIocpBuffer* pBuffer = m_freeBuffer.construct();

	if (!pBuffer)
	{
		WriteErrorLog(	_T("[AllocateNewBuffer]:Allocate faled."),
						0 );
	}

	if (!pBuffer->AllocateBuffer(nAllocSize))
	{
		ClearIocpBuffer(pBuffer);
		pBuffer = NULL;
	}

	return pBuffer;
}

VOID CDIocp::ClearIocpBuffer(CDIocpBuffer* pBuffer)
{
	if (!pBuffer)
		{ return; }

	//Clear buffer.
	pBuffer->DeAllocateBuffer();
	m_freeBuffer.destroy(pBuffer);
}

VOID CDIocp::ProcessIoMessage(ClientContext* pContext, CDIocpBuffer* pIocpBuffer, BYTE bType, DWORD dwIoSize)
{
	switch(bType)
	{
	case IoReadCompleted:
		OnIoReadCompleted(pContext, pIocpBuffer, dwIoSize);
		break;

	case IoZeroReadCompleted:
		OnIoZeroReadCompleted(pContext, pIocpBuffer);
		break;

	case IoWrite:
		OnIoWrite(pContext, pIocpBuffer);
		break;

	case IoWriteCompleted:
		OnIoWriteCompleted(pContext, pIocpBuffer, dwIoSize);
		break;

	case IoInit:
		OnIoInit(pContext, pIocpBuffer);
		break;

	default:
		break;
	}
}

VOID CDIocp::OnIoInit(ClientContext* pContext, CDIocpBuffer* pIocpBuffer)
{
	//Add into ClientContext map.
	AddClientContext(pContext);

	if (pContext->m_pHandler)
	{
		pContext->m_pHandler->OnConn(pContext);
	}

	//Post IoZeroByteRead to avoid WSANOBUFS error. 
	//when data coming, zero WSARecv will return.
	OnZeroRecv(pContext, pIocpBuffer);
}

VOID CDIocp::OnIoReadCompleted(ClientContext* pContext, CDIocpBuffer* pIocpBuffer, UINT nRecv)
{
	if (!IsActive(pContext))
	{
		ReleaseClientContext(pContext);
		ClearIocpBuffer(pIocpBuffer);

		return;
	}

	if (!nRecv)
	{
		//Recv zero byte data,error dump.
		DisConnectClient(pContext);
		ReleaseClientContext(pContext);
		ClearIocpBuffer(pIocpBuffer);

		return;
	}

	if (nRecv && pContext->m_pHandler)
	{
		pContext->m_pHandler->OnRecv(pContext, pIocpBuffer->GetBuffer(), nRecv);
	}

	//Post next ZeroByteRead, reusing IocpBuffer.
	OnZeroRecv(pContext, pIocpBuffer);
}

VOID CDIocp::OnIoZeroReadCompleted(ClientContext* pContext, CDIocpBuffer* pIocpBuffer)
{
	if (!IsActive(pContext))
	{
		ReleaseClientContext(pContext);
		ClearIocpBuffer(pIocpBuffer);

		return;
	}
	//Post real WSARecv(),wait for OnIoReadCompleted.
	OnRecv(pContext, pIocpBuffer);
}

VOID CDIocp::OnIoWrite(ClientContext* pContext, CDIocpBuffer* pIocpBuffer)
{
	if (!IsActive(pContext))
	{
		ReleaseClientContext(pContext);
		ClearIocpBuffer(pIocpBuffer);

		return;
	}
	//Get send buffer from send map.
	pIocpBuffer = GetNextSendBuffer(pContext, pIocpBuffer);

	while (pIocpBuffer)
	{
		pIocpBuffer->SetOperation(IoWriteCompleted);

		ULONG ulFlags = MSG_PARTIAL;
		DWORD dwSendNumBytes = 0;

		int nRetVal = WSASend(	pContext->m_Socket, 
								&pIocpBuffer->m_stWsaBuf,
								1,
								&dwSendNumBytes, 
								ulFlags,
								&pIocpBuffer->m_stOverLapped, 
								NULL
								);
		if (nRetVal == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) 
		{
			//WSASend Error dump.
			DisConnectClient(pContext);
			ReleaseClientContext(pContext);
			ClearIocpBuffer(pIocpBuffer);

			WriteErrorLog(	_T("[OnIoWrite]:WSASend failed."),
							DWORD(pContext)
							);

			break;
		}

		IncreaseSendSequence(pContext);
		pIocpBuffer = GetNextSendBuffer(pContext, NULL);
	}
}

VOID CDIocp::OnIoWriteCompleted(ClientContext* pContext, CDIocpBuffer* pIocpBuffer, UINT nSend)
{
	if (!IsActive(pContext))
	{
		ReleaseClientContext(pContext);
		ClearIocpBuffer(pIocpBuffer);

		return;
	}

	if (!nSend || nSend != pIocpBuffer->GetUsed())
	{
		WriteErrorLog(	_T("[OnIoWriteCompleted]:Send only zero byte."),
						WSAGetLastError()
						);
		//Send buff not completed.
		DisConnectClient(pContext);
	}

	if (pContext->m_pHandler)
	{
		pContext->m_pHandler->OnSent(	pContext, 
										pIocpBuffer->GetHandle(), 
										pIocpBuffer->GetBuffer(), 
										nSend
										);
	}

	ClearIocpBuffer(pIocpBuffer);
	ReleaseClientContext(pContext);
}

VOID CDIocp::IncreasePendingIO(ClientContext* pContext)
{
	pContext->m_LockContext.Lock();
	++pContext->m_IoPending;
	pContext->m_LockContext.Unlock();
}

int CDIocp::DecreasePendingIO(ClientContext* pContext)
{
	int nIOPending = 0;

	pContext->m_LockContext.Lock();
	nIOPending = --pContext->m_IoPending;
	pContext->m_LockContext.Unlock();

	return nIOPending;
}

VOID CDIocp::MakeSendInOrder(ClientContext* pContext, CDIocpBuffer* pIocpBuffer)
{
	pContext->m_LockContext.Lock();

	pIocpBuffer->m_dwSerial = pContext->m_SendSerial;
	pContext->m_SendSerial = (++pContext->m_SendSerial)%SEND_SENQUENCE_NUMBER;

	pContext->m_LockContext.Unlock();
}

CDIocpBuffer* CDIocp::GetNextSendBuffer(ClientContext* pContext, CDIocpBuffer* pIocpBuffer)
{
	pContext->m_LockContext.Lock();

	if (pIocpBuffer)
	{
		if(pIocpBuffer->m_dwSerial != pContext->m_SentSerial)
		{
			pContext->m_SendMap[pIocpBuffer->m_dwSerial] = pIocpBuffer;

			BufferMap::iterator iter = pContext->m_SendMap.find(pContext->m_SentSerial);
			if(iter != pContext->m_SendMap.end())
			{
				pIocpBuffer = (*iter).second;
				pContext->m_SendMap.erase(iter);
			}
			else
			{
				pIocpBuffer = NULL;
			}
		}
	}
	else
	{
		BufferMap::iterator iter = pContext->m_SendMap.find(pContext->m_SentSerial);
		if(iter != pContext->m_SendMap.end())
		{
			pIocpBuffer = (*iter).second;
			pContext->m_SendMap.erase(iter);
		}
	}

	pContext->m_LockContext.Unlock();

	return pIocpBuffer;
}

VOID CDIocp::MakeRecvInOrder(ClientContext* pContext, CDIocpBuffer* pIocpBuffer)
{
	pContext->m_LockContext.Lock();

	pIocpBuffer->m_dwSerial = pContext->m_RecvSerial;
	pContext->m_RecvSerial = (++pContext->m_RecvSerial)%SEND_SENQUENCE_NUMBER;

	pContext->m_LockContext.Unlock();
}

CDIocpBuffer* CDIocp::GetNextRecvBuffer(ClientContext* pContext, CDIocpBuffer* pIocpBuffer)
{
	pContext->m_LockContext.Lock();

	if(NULL != pIocpBuffer)
	{
		if(pIocpBuffer->m_dwSerial == pContext->m_RcvdSerial)
		{
			pContext->m_RcvdSerial = (++pContext->m_RcvdSerial)%SEND_SENQUENCE_NUMBER;
		}
		else
		{
			pContext->m_RecvMap[pIocpBuffer->m_dwSerial] = pIocpBuffer;

			BufferMap::iterator iter = pContext->m_RecvMap.find(pContext->m_RcvdSerial);

			if(iter != pContext->m_RecvMap.end())
			{
				pIocpBuffer = (*iter).second;
				pContext->m_RecvMap.erase(iter);

				pContext->m_RcvdSerial = (++pContext->m_RcvdSerial)%SEND_SENQUENCE_NUMBER;
			}
			else
			{
				pIocpBuffer = NULL;
			}
		}
	}
	else
	{
		BufferMap::iterator iter = pContext->m_RecvMap.find(pContext->m_RcvdSerial);

		if(iter != pContext->m_RecvMap.end())
		{
			pIocpBuffer = (*iter).second;
			pContext->m_RecvMap.erase(iter);

			pContext->m_RcvdSerial = (++pContext->m_RcvdSerial)%SEND_SENQUENCE_NUMBER;
		}
	}

	pContext->m_LockContext.Unlock();

	return pIocpBuffer;
}

BOOL CDIocp::Send(int nID, HANDLE hSend, LPBYTE bSendBuff, DWORD dwSendSize)
{
	ClientContext* pContext = FindClientContext(nID);

	if (!pContext) 
	{
		return FALSE;
	}

	return OnSend(pContext, hSend, bSendBuff, dwSendSize);
}

BOOL CDIocp::OnSend(ClientContext* pContext, HANDLE hSend, LPBYTE bSendBuff, DWORD dwSendSize)
{
	CDIocpBuffer* pIocpBuffer = AllocateNewBuffer(0);
	
	if (!pIocpBuffer)
	{
		DisConnectClient(pContext);
		ReleaseClientContext(pContext);
		return FALSE;
	}

	pIocpBuffer->SetupWrite(hSend, bSendBuff, dwSendSize);

	return OnSend(pContext, pIocpBuffer);
}

BOOL CDIocp::OnSend(ClientContext* pContext, CDIocpBuffer* pIocpBuffer)
{
	if(	!pContext || !pIocpBuffer)
	{
		ClearIocpBuffer(pIocpBuffer);

		return FALSE;
	}
	//Add into SendList, to make sure send in order.
	MakeSendInOrder(pContext, pIocpBuffer);

	PostIoMessage(pContext, pIocpBuffer, IoWrite);

	return TRUE;
}

BOOL CDIocp::OnRecv(ClientContext* pContext, CDIocpBuffer* pIocpBuffer)
{
	if (!pContext || !pIocpBuffer)
	{
		ClearIocpBuffer(pIocpBuffer);

		return FALSE;
	}

	//Post real read operation.
	pIocpBuffer->SetOperation(IoReadCompleted);
	pIocpBuffer->SetupRead();
	DWORD dwIoSize=0;
	ULONG ulFlags = MSG_PARTIAL;

	int nRetVal = WSARecv(	pContext->m_Socket, 
							&pIocpBuffer->m_stWsaBuf,
							1,
							&dwIoSize, 
							&ulFlags,
							&pIocpBuffer->m_stOverLapped, 
							NULL
							);

	if (nRetVal == SOCKET_ERROR && GetLastError() != WSA_IO_PENDING)
	{
		//Error dump.
		DisConnectClient(pContext);
		ReleaseClientContext(pContext);
		ClearIocpBuffer(pIocpBuffer);

		return FALSE;
	}

	return TRUE;
}

BOOL CDIocp::OnZeroRecv(ClientContext* pContext, CDIocpBuffer* pIocpBuffer)
{
	if (!pContext)
	{
		return FALSE;
	}

	if (!pIocpBuffer)
	{
		//Null buffer for zero read.
		pIocpBuffer = AllocateNewBuffer();

		if (!pIocpBuffer)
		{
			ReleaseClientContext(pContext);
			return FALSE;
		}
	}

	pIocpBuffer->SetOperation(IoZeroReadCompleted);
	pIocpBuffer->SetupZeroRead();
	DWORD dwIoSize=0;
	ULONG ulFlags = MSG_PARTIAL;

	int nRetVal = WSARecv(	pContext->m_Socket, 
							&pIocpBuffer->m_stWsaBuf,
							1,
							&dwIoSize, 
							&ulFlags,
							&pIocpBuffer->m_stOverLapped, 
							NULL
							);

	if (nRetVal == SOCKET_ERROR && GetLastError() != WSA_IO_PENDING)
	{
		//Error dump.
		DisConnectClient(pContext);
		ReleaseClientContext(pContext);
		ClearIocpBuffer(pIocpBuffer);

		return FALSE;
	}

	return TRUE;
}

BOOL CDIocp::XPNTSYNFloodProtection(int iValue,
									int iTcpMaxHalfOpen, 
									int iTcpMaxHalfOpenRetried, 
									int iTcpMaxPortsExhausted,
									int iTcpMaxConnectResponseRetransmissions
									)
{
	const TCHAR sKey_PATH[]									= _T("\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters");
	const TCHAR sKey_SynAttackProtect[]						= _T("SynAttackProtect");
	const TCHAR sKey_TcpMaxHalfOpen[]						= _T("TcpMaxHalfOpen");
	const TCHAR sKey_TcpMaxHalfOpenRetried[]				= _T("TcpMaxHalfOpenRetried");
	const TCHAR sKey_TcpMaxPortsExhausted[]					= _T("TcpMaxPortsExhausted");
	const TCHAR sKey_TcpMaxConnectResponseRetransmissions[]	= _T("TcpMaxConnectResponseRetransmissions");

	HKEY    hKey;
	DWORD   val		= 0;
	LONG	r		= 0;
	BOOL	bRet	= TRUE;

	// Set the sKey_SynAttackProtect
	val=iValue;
	if (RegOpenKey(HKEY_LOCAL_MACHINE, sKey_PATH, &hKey) != ERROR_SUCCESS)
		if (RegCreateKey(HKEY_LOCAL_MACHINE, sKey_SynAttackProtect, &hKey) != ERROR_SUCCESS)
			return FALSE;
	r = RegSetValueEx(hKey, sKey_SynAttackProtect, 0, REG_DWORD, (BYTE *)&val, sizeof(val));
	RegCloseKey(hKey);
	bRet&= (r == ERROR_SUCCESS);

	// Special Parameters is used.
	if(iValue==1)
	{
		// Set the sKey_SynAttackProtect
		val=iTcpMaxHalfOpenRetried;
		if (RegOpenKey(HKEY_LOCAL_MACHINE, sKey_PATH, &hKey) != ERROR_SUCCESS)
			if (RegCreateKey(HKEY_LOCAL_MACHINE, sKey_TcpMaxHalfOpen, &hKey) != ERROR_SUCCESS)
				return FALSE;
		r = RegSetValueEx(hKey, sKey_TcpMaxHalfOpen, 0, REG_DWORD, (BYTE *)&val, sizeof(val));
		RegCloseKey(hKey);
		bRet&= (r == ERROR_SUCCESS);

		// Set the sKey_TcpMaxHalfOpenRetried
		val=iTcpMaxHalfOpenRetried;
		if (RegOpenKey(HKEY_LOCAL_MACHINE, sKey_PATH, &hKey) != ERROR_SUCCESS)
			if (RegCreateKey(HKEY_LOCAL_MACHINE, sKey_TcpMaxHalfOpenRetried, &hKey) != ERROR_SUCCESS)
				return FALSE;
		r = RegSetValueEx(hKey, sKey_TcpMaxHalfOpenRetried, 0, REG_DWORD, (BYTE *)&val, sizeof(val));
		RegCloseKey(hKey);
		bRet&= (r == ERROR_SUCCESS);


		// Set the sKey_TcpMaxPortsExhausted
		val=iTcpMaxPortsExhausted;
		if (RegOpenKey(HKEY_LOCAL_MACHINE, sKey_PATH, &hKey) != ERROR_SUCCESS)
			if (RegCreateKey(HKEY_LOCAL_MACHINE, sKey_TcpMaxPortsExhausted, &hKey) != ERROR_SUCCESS)
				return FALSE;
		r = RegSetValueEx(hKey, sKey_TcpMaxPortsExhausted, 0, REG_DWORD, (BYTE *)&val, sizeof(val));
		RegCloseKey(hKey);
		bRet&= (r == ERROR_SUCCESS);


		// Set sKey_TcpMaxConnectResponseRetransmissions
		val=iTcpMaxConnectResponseRetransmissions;
		if (RegOpenKey(HKEY_LOCAL_MACHINE, sKey_PATH, &hKey) != ERROR_SUCCESS)
			if (RegCreateKey(HKEY_LOCAL_MACHINE, sKey_TcpMaxConnectResponseRetransmissions, &hKey) != ERROR_SUCCESS)
				return FALSE;
		r = RegSetValueEx(hKey, sKey_TcpMaxConnectResponseRetransmissions, 0, REG_DWORD, (BYTE *)&val, sizeof(val));
		RegCloseKey(hKey);
		bRet&= (r == ERROR_SUCCESS);

	}

	return bRet;
}

int CDIocp::ConnectAcceptCondition(	LPWSABUF	lpCallerId,
									LPWSABUF	lpCallerData,
									LPQOS		lpSQOS,
									LPQOS		lpGQOS,
									LPWSABUF	lpCalleeId, 
									LPWSABUF	lpCalleeData,
									GROUP 		*g,
									DWORD		dwCallbackData
									)
{
	sockaddr_in* pCaller = (sockaddr_in*)lpCallerId->buf;
	sockaddr_in* pCallee = (sockaddr_in*)lpCalleeId->buf;

	CDIocp* pThis = reinterpret_cast<CDIocp*>(dwCallbackData);

	// Fixed 2015/6/25, for test unknown connection error.
	//if (pThis->m_nConnection >= pThis->m_nMaxConnection)
	//{
	//	return CF_REJECT;
	//}

#ifndef IOCP_MFCDLLSERVER
	//if (pThis->m_BanIp.IsInsideFilter(pCaller))
	//{
		// Do not send ACK, the attacker do not know if the server exist or not. 
		//return CF_REJECT;
	//}
#endif // IOCP_MFCDLLSERVER

	return CF_ACCEPT;
}

BOOL CDIocp::WriteErrorLog(LPCTSTR lpszLog, DWORD dwError)
{
	OutputDebugString(lpszLog);

	return TRUE;
}

#ifndef IOCP_MFCDLLSERVER

VOID CDIocp::AddIntoFilter(sockaddr_in* pAddr)
{
	//this->m_BanIp.AddIntoFilter(pAddr);
}

BOOL CDIocp::IsInsideFilter(sockaddr_in* pAddr)
{
	//return this->m_BanIp.IsInsideFilter(pAddr);
	return true;
}

VOID CDIocp::RemoveFromFilter(sockaddr_in* pAddr)
{
	//this->m_BanIp.RemoveFromFilter(pAddr);
}

int CDIocp::GetBanIp(LPBYTE bData, int nSize)
{
	//return this->m_BanIp.GetBanIp(bData, nSize);
	return 0;
}

VOID CDIocp::SaveBanIp()
{
	//this->m_BanIp.WriteBanIpToFile();
}

#endif // #ifndef IOCP_MFCDLLSERVER

void CDIocp::SetTimer(int nID, int nMillisecond, bool reset)
{
	ClientContext* pContext = FindClientContext(nID);

	if (pContext) {
		pContext->m_pTimer = m_Timer.AddTimer(nID, nMillisecond, reset, this, MyTimerProc);
	}
}

void CDIocp::KillTimer(int nID)
{
	ClientContext* pContext = FindClientContext(nID);

	if (pContext && pContext->m_pTimer) {
		m_Timer.RemoveTimer(pContext->m_pTimer);
		pContext->m_pTimer = NULL;
	}
}

DWORD CDIocp::ThreadIoWorkProc(LPVOID lPvoid)
{
	CDIocp* pThis = reinterpret_cast<CDIocp*>(lPvoid);

	if (!pThis) { 
		return 0xdead; 
	}

	DWORD			dwIoSize		= 0;
	ClientContext*	pClientContext	= NULL;
	CDIocpBuffer*	pIocpBuffer		= NULL;
	BOOL			bError			= FALSE;
	BOOL			bResult			= FALSE;
	HANDLE			hCompletionPort = pThis->m_hCompletionPort;
	LPOVERLAPPED	pOverlapped		= NULL;

	while (TRUE)
	{
		bResult = ::GetQueuedCompletionStatus(	hCompletionPort,
												&dwIoSize,
												(PULONG_PTR)&pClientContext,
												&pOverlapped,
												INFINITE
												);

		if (!bResult) {
			DWORD dwError = ::GetLastError();
			//Error dump.
			pThis->WriteErrorLog(	_T("[ThreadIoWorkProc]:GetQueuedCompletionStatus return false."),
									::GetLastError()
									);

			if (pClientContext) {
				pThis->DisConnectClient(pClientContext);
				pThis->ReleaseClientContext(pClientContext);
			}

			if (pOverlapped) {
				pIocpBuffer = CONTAINING_RECORD(pOverlapped, CDIocpBuffer, m_stOverLapped);

				pThis->ClearIocpBuffer(pIocpBuffer);
			}
		} else {
			if (!pClientContext && !pOverlapped) {
				if (!pThis->m_bServerRun) {
					//Decrease number of runing IoWorkerThread until == 0.
					::InterlockedDecrement(&pThis->m_nRunIoWork);

					if (!pThis->m_nRunIoWork) {
						SetEvent(pThis->m_hEventIoWork);
					} else {
						pThis->PostExitMessage();
					}
					OutputDebugString(_T("ThreadIoWork exit.\r\n"));
					break;
				}
			} else {
				if (pOverlapped) {
					pIocpBuffer = CONTAINING_RECORD(pOverlapped, CDIocpBuffer, m_stOverLapped);
					pThis->ProcessIoMessage(pClientContext, 
											pIocpBuffer, 
											pIocpBuffer->GetIoType(), 
											dwIoSize
											);
				} else {
					if (pClientContext) {
						pThis->DisConnectClient(pClientContext);
						pThis->ReleaseClientContext(pClientContext);
					}
				}
			}
		}
		pClientContext	= NULL;
		pOverlapped		= NULL;
	}

	return 0xdead;
}

DWORD CDIocp::ThreadListenProc(LPVOID lPvoid)
{
	CDIocp* pThis = reinterpret_cast<CDIocp*>(lPvoid);

	if (!pThis) { 
		return 0xdead; 
	}

	int	nResult = 0;
	int nLength = sizeof(SOCKADDR);
	DWORD dwResult = 0;
	WSANETWORKEVENTS wsaEvents;

	while(pThis->m_bServerRun)
	{
		//Do a loop.
		dwResult = WSAWaitForMultipleEvents(1, &pThis->m_hEventListen, FALSE, 100, FALSE);

		if (WSA_WAIT_TIMEOUT == dwResult) {
			continue;
		}

		nResult = WSAEnumNetworkEvents(pThis->m_sockListen, pThis->m_hEventListen, &wsaEvents);

		if (SOCKET_ERROR == nResult) {
			break;
		}

		if (wsaEvents.lNetworkEvents & FD_ACCEPT) {

			if(wsaEvents.iErrorCode[FD_ACCEPT_BIT] == 0 && pThis->m_bServerRun) {

				SOCKET sockClient = INVALID_SOCKET;

				sockClient = WSAAccept(	pThis->m_sockListen,
										NULL,
										&nLength,
										ConnectAcceptCondition,
										(DWORD_PTR)pThis
										);

				if(INVALID_SOCKET == sockClient) {
					pThis->WriteErrorLog(	_T("[ThreadListenProc]:WSAAccept INVALID_SOCKET."),
											::WSAGetLastError()
											);
				} else {
					//Accept new coming connetion.
					pThis->BindToClientContext(sockClient);
				}
			}
		}
	}

	SetEvent(pThis->m_hEventRun);
	OutputDebugString(_T("ThreadListenProc exit.\r\n"));

	return 0xdead;
}

void CDIocp::MyTimerProc(int nID, void* param)
{
	CDIocp* pThis = reinterpret_cast<CDIocp*>(param);

	ClientContext* pContext = pThis->FindClientContext(nID);

	if (pContext && pContext->m_pHandler)
	{
		pContext->m_pHandler->OnTimer(pContext);
	}
}