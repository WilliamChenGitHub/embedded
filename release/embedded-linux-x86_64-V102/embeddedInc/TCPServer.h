#ifndef __TCPSERVER_H__
#define __TCPSERVER_H__

#include "HashTable.h"
#include "ThreadPool.h"
#include "Communication.h"
#include "TCPTransferFactory.h"



#define MAX_MONITOR_EVENTS_EVERY_TIME 128

typedef struct __tcpSer
{
    SOCKET_T socket;
    HASH_TABLE_ST *pCHTbl; // connection hash table
    THREAD_POOL_ST *pThP; // Thread pool
    int32_t monitor;

    int32_t monitorFd;
    struct epoll_event evtArr[MAX_MONITOR_EVENTS_EVERY_TIME];
    uint8_t buf[1024];

    COM_PACK_PARSE_FT pParseCb;
    uint32_t usrDataSz;
}TCP_SERVER_ST;


typedef struct
{
    HASH_NODE_ST hasNode;
    COM_PORT_ATTR_ST comPort;
    uint8_t usrDatBuf[0];
}CONNECTION_NODE_ST;


#ifdef __cplusplus
extern "C"{
#endif

TCP_SERVER_ST *TcpServerCreate(const char *ipaddr, const uint16_t port, COM_PACK_PARSE_FT pParseCb, uint32_t threadNb, uint32_t threadStkSz, uint32_t connTblSz, uint32_t usrDataSz);

void TcpServerDestory(TCP_SERVER_ST *pTCPSer);

#ifdef __cplusplus
}
#endif



#endif


