#ifndef __TCPTRANSFER_H__
#define __TCPTRANSFER_H__

#include "Transfer.h"

#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <sys/epoll.h>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>


#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>


typedef struct
{
    TRANSFER_ST transfer;
    MUTEX_T txMutex;
    SOCKET_T socket;
    THREAD_T clientRxTh;
    bool exit;
}TCP_TRANSFER_ST;


#ifdef __cplusplus
extern "C"{
#endif

int32_t TCPTransferInit(TRANSFER_ST *pTrans, SOCKET_T socket, bool isClient);

#ifdef __cplusplus
}
#endif

#endif



