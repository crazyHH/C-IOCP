/**
 *	ģ��: CDHandler
 *
 *	����: �������ӵ����лص��ӿڶ���
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

	//Э��ƥ��
	virtual int IsKindof(ClientContext *pContext) = 0;
	//��������
	virtual void OnConn(ClientContext *pContext) = 0;
	//���ӹر�
	virtual void OnClose(ClientContext *pContext) = 0;
	//��ʱ���¼�
	virtual void OnTimer(ClientContext *pContext) = 0;
	//���ݷ������
	virtual void OnSent(ClientContext *pContext, void *pSend, unsigned char* pData, unsigned int nSent) = 0;
	//��������
	virtual void OnRecv(ClientContext *pContext, unsigned char* pData, unsigned int nRecv) = 0;
};

#endif // _CDLIB_HANDLER_H