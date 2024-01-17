#include "TCPTransferFactory.h"

TRANSFER_ST *TCPTransferCreate(TRANSFER_FACTORY_ST *pTransFactory)
{
    TCP_TRANSFER_FACTORY_ST *pTCPTransferFactory = (TCP_TRANSFER_FACTORY_ST *)pTransFactory;
    TRANSFER_ST *pTransfer = MmMngrMalloc(sizeof(TCP_TRANSFER_ST));
    
    do
    {
        if(!pTransfer)
        {
            LOGE("Creat Transfer failed\r\n");
            break;
        }
        memset(pTransfer, 0, sizeof(TCP_TRANSFER_ST));
        
        if(0 != TCPTransferInit(pTransfer,pTCPTransferFactory->socket, pTCPTransferFactory->isClient))
        {
            LOGE("TCP transfer init failed\r\n");
            break;
        }
        
        return pTransfer;
    }while(0);

    if(pTransfer)
    {
        MmMngrFree(pTransfer);
    }
    
    return NULL;
}



VOID_T TCPTransferFactoryInit(TRANSFER_FACTORY_ST *pFact, SOCKET_T socket, BOOL_T isClient)
{
    TCP_TRANSFER_FACTORY_ST *pTCPTransferFactory = (TCP_TRANSFER_FACTORY_ST *)pFact;

    static TRANSFER_FACTORY_VTBL_ST tcpTransFacotryVtbl = 
    {
        TCPTransferCreate
    };
    
    pTCPTransferFactory->transferFacotry.vPtr = &tcpTransFacotryVtbl;
    pTCPTransferFactory->socket = socket;
    pTCPTransferFactory->isClient = isClient;
}





