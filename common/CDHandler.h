/**
 *	模块: CDHandler
 *
 *	概述: 网络连接的所有回调接口定义
 *
 */
#ifndef _CDLIB_HANDLER_H
#define _CDLIB_HANDLER_H

struct ClientContext;

class CDHandler
{
public:
	CDHandler(){};
	virtual ~CDHandler(){};

	//协议匹配
	virtual int IsKindof(ClientContext *pContext) = 0;
	//接收连接
	virtual void OnConn(ClientContext *pContext) = 0;
	//连接关闭
	virtual void OnClose(ClientContext *pContext) = 0;
	//定时器事件
	virtual void OnTimer(ClientContext *pContext) = 0;
	//数据发送完成
	virtual void OnSent(ClientContext *pContext, void *pSend, unsigned char* pData, unsigned int nSent) = 0;
	//接收数据
	virtual void OnRecv(ClientContext *pContext, unsigned char* pData, unsigned int nRecv) = 0;
};

#endif // _CDLIB_HANDLER_H