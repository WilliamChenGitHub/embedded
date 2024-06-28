#ifndef __TCPCLIENT_H__
#define __TCPCLIENT_H__

#include "ThreadPool.h"
#include "Communication.h"
#include "TCPTransferFactory.h"

typedef struct __tcpCli
{
    SOCKET_T socket;
    
    COM_ATTR_ST *pCom;
    COM_PORT_ATTR_ST comPort;
}TCP_CLIENT_ST;



#ifdef __cplusplus
extern "C"{
#endif

TCP_CLIENT_ST *TcpClientCreate(const char *server, const uint16_t port, COM_PACK_PARSE_FT pParseCb);

void TcpClientDestory(TCP_CLIENT_ST *pTCPCli);

#ifdef __cplusplus
}
#endif



#endif


