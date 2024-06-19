#ifndef __TCPTRANSFERFACTORY_H__
#define __TCPTRANSFERFACTORY_H__

#include "TransferFactory.h"
#include "TCPTransfer.h"

typedef struct __TCPTransferFactory
{
    TRANSFER_FACTORY_ST transferFacotry;
    SOCKET_T socket;
    bool isClient;
}TCP_TRANSFER_FACTORY_ST;


#ifdef __cplusplus
extern "C"{
#endif

void TCPTransferFactoryInit(TRANSFER_FACTORY_ST *pFact, SOCKET_T socket, bool isClient);


#ifdef __cplusplus
}
#endif

#endif

