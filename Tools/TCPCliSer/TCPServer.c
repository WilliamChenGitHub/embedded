#include "TCPServer.h"
#include "MmMngr.h"

static void sig_handler(int signum)
{
    if(signum == SIGPIPE)
    {
        LOGI("recive SIGPIPE signal...\n");
    }
}

static void ConnectionNodeDestory(void *pArg)
{
    CONNECTION_NODE_ST *pConnectNode = (CONNECTION_NODE_ST *)pArg;

    CommunicationDeinit(pConnectNode->pCom);
    MmMngrFree(pConnectNode);
}

static void ServerMonitorMission(void *pArg)
{
    TCP_SERVER_ST *pTCPSer = (TCP_SERVER_ST *)pArg;
    int32_t evts = 0;

    struct sockaddr_in cliAddr;
    socklen_t cliAddLen = sizeof(cliAddr);
    SOCKET_T cliSocket = 0;
    
    struct epoll_event ev;

    TCP_TRANSFER_FACTORY_ST tcpTransferFact = {0};

    while(pTCPSer->monitor)
    {
        evts = epoll_wait(pTCPSer->monitorFd, pTCPSer->evtArr, MAX_MONITOR_EVENTS_EVERY_TIME, 50);
        if(0 >= evts)
        {
            if(pTCPSer->monitor)
            {
                continue;
            }
        }

        LOGD("new evt = %d\r\n", evts);
        for(int32_t i = 0; i < evts; i++)
        {
            if(pTCPSer->evtArr[i].data.fd == pTCPSer->socket) // new connection
            {
                cliSocket = accept4(pTCPSer->socket, (struct sockaddr *)&cliAddr, &cliAddLen, SOCK_NONBLOCK);
                if(0 < cliSocket)
                {
                    LOGD("new client connection ...\n");
                    LOGD("client ip:%s port:%d\n", inet_ntoa(cliAddr.sin_addr), ntohs(cliAddr.sin_port));

                    ev.events = EPOLLIN | EPOLLET;
                    ev.data.fd = cliSocket;
                    
                    if(0 > epoll_ctl(pTCPSer->monitorFd, EPOLL_CTL_ADD, cliSocket, &ev))
                    {
                        LOGE("add client socket failed\r\n");
                        goto err1;
                    }

                    CONNECTION_NODE_ST *pNewConnectionNode = MmMngrMalloc(sizeof *pNewConnectionNode);
                    if(!pNewConnectionNode)
                    {
                        LOGE("malloc new connection failed\r\n");
                        goto err2;
                    }

                    pNewConnectionNode->hasNode.pNodeDestory = ConnectionNodeDestory;
                    pNewConnectionNode->hasNode.key = cliSocket;

                    TCPTransferFactoryInit((TRANSFER_FACTORY_ST *)&tcpTransferFact, cliSocket, false);
                    pNewConnectionNode->pCom = CommunicationInit((TRANSFER_FACTORY_ST *)&tcpTransferFact, false, false);
                    if(!pNewConnectionNode->pCom)
                    {
                        goto err3;
                    }
                    CommunicationRegPort(pNewConnectionNode->pCom, &pNewConnectionNode->comPort, pTCPSer->pParseCb, 1);
                    
                    if(HashTableInsert(pTCPSer->pCHTbl, &pNewConnectionNode->hasNode))
                    {
                        goto err4;
                    }

                    continue;

                    err4:
                        CommunicationDeinit(pNewConnectionNode->pCom);

                    err3:
                        MmMngrFree(pNewConnectionNode);

                    err2:
                        epoll_ctl(pTCPSer->monitorFd, EPOLL_CTL_DEL, cliSocket, NULL);

                    err1:
                        close(cliSocket);
                }
            }
            else // client message
            {
                cliSocket = pTCPSer->evtArr[i].data.fd;
                CONNECTION_NODE_ST *pNode = (CONNECTION_NODE_ST *)HashTableLookup(pTCPSer->pCHTbl, cliSocket);
                bool disconnect = false;
                if(pNode)
                {
                    COM_ATTR_ST *pCom = (COM_ATTR_ST *)pNode->pCom;

                    do
                    {
                        int32_t rxBytes = recv(cliSocket, pTCPSer->buf, sizeof(pTCPSer->buf), 0);
                        
                        LOGD("read len = %d, %d\r\n", rxBytes, errno);
                        
                        if(0 < rxBytes)
                        {
                            TransferRx(pCom->pTransfer, pTCPSer->buf, rxBytes);
                        }
                        else if(0 == rxBytes)
                        {
                            if(EAGAIN == errno || EWOULDBLOCK == errno)
                            {
                            #if 0
                                struct tcp_info info = {0};
                                int len = sizeof(info);
                                getsockopt(cliSocket, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&len); 
                                if(TCP_CLOSE_WAIT == info.tcpi_state)
                                {
                                    LOGI("Client disconnection!\n");
                                    disconnect = true;
                                }
                            #else
                                disconnect = true;
                            #endif
                            }
                            break;
                        }
                        else
                        {
                            break;
                        }
                    }
                    while (1);
                    ThreadPoolDispatchMission(pTCPSer->pThP, (THREAD_POOL_MISSION)CommunicationTryParse, pCom);
                    if(disconnect)
                    {
                        LOGD("disconnect = %x\r\n", pNode->hasNode.key);
                        HashTableDelet(pTCPSer->pCHTbl, pNode->hasNode.key);
                        epoll_ctl(pTCPSer->monitorFd, EPOLL_CTL_DEL, cliSocket, NULL);
                        
                        close(pTCPSer->evtArr[i].data.fd);
                    }
                }
            }
        }
    }
}



TCP_SERVER_ST *TcpServerCreate(const char *ipaddr, const uint16_t port, COM_PACK_PARSE_FT pParseCb, uint32_t threadNb, uint32_t threadStkSz, uint32_t connTblSz)
{
    TCP_SERVER_ST *pTCPSer = NULL;
    struct epoll_event ev = {0};
    U64_T on = 1;

    pTCPSer = MmMngrMalloc(sizeof *pTCPSer);
    if(!pTCPSer)
    {
        LOGE("Malloc TCP server struct failed\r\n");
        goto err1;
    }

    signal(SIGPIPE, sig_handler);

    pTCPSer->socket = socket(AF_INET, SOCK_STREAM, 0); // create socket
    if(-1 == pTCPSer->socket)
    {
        LOGE("create socket error: %s\r\n", strerror(errno));
        goto err2;
    }

    setsockopt(pTCPSer->socket, SOL_SOCKET, SO_REUSEPORT, (void *)&on, sizeof(on));
    setsockopt(pTCPSer->socket, SOL_SOCKET, SO_REUSEADDR, (void *)&on, sizeof(on));

    S32_T flags = fcntl(pTCPSer->socket, F_GETFL, 0);
    if (flags == -1)
    {
        LOGE("Fcntl get failed\r\n");
        goto err3;
    }

    if(fcntl(pTCPSer->socket, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        LOGE("Fcntl set failed\r\n");
        goto err3;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;

    if(ipaddr)
    {
        addr.sin_addr.s_addr = inet_addr(ipaddr);
    }
    else
    {
        addr.sin_addr.s_addr = INADDR_ANY; // bind all network card ip
    }
    addr.sin_port = htons(port);//把主机端口号转为网络端口号

    if(bind(pTCPSer->socket, (struct sockaddr *)&addr, sizeof(addr)))//绑定ip和端口号
    {
        LOGE("bind error: %s\r\n", strerror(errno));
        goto err3;
    }

    if(listen(pTCPSer->socket, 10))//监听套接字
    {
        LOGE("listen error: %s\r\n", strerror(errno));
        goto err3;
    }

    pTCPSer->pCHTbl = HashTableCreate(connTblSz);
    if(!pTCPSer->pCHTbl)
    {
        LOGE("connection hash table create failed\r\n");
        goto err3;
    }

    pTCPSer->pThP = ThreadPoolCreate(threadNb, threadStkSz);
    if(!pTCPSer->pThP)
    {
        LOGE("thread pool create failed\r\n");
        goto err4;
    }

    //epoll
    pTCPSer->monitorFd = epoll_create(1);
    if(0 > pTCPSer->monitorFd)
    {
        LOGE("epoll create failed\r\n");
        goto err5;
    }

    ev.events = EPOLLIN; // | EPOLLET;
    ev.data.fd = pTCPSer->socket;

    if(0 > epoll_ctl(pTCPSer->monitorFd, EPOLL_CTL_ADD, pTCPSer->socket, &ev))
    {
        LOGE("epoll ctl failed\r\n");
        goto err6;
    }

    pTCPSer->pParseCb = pParseCb;
    pTCPSer->monitor = 1;
    if(ThreadPoolDispatchMission(pTCPSer->pThP, ServerMonitorMission, pTCPSer)) // start monitor server
    {
        LOGE("start server monitor mission failed\r\n");
        goto err7;
    }

    return pTCPSer;

err7:
    epoll_ctl(pTCPSer->monitorFd, EPOLL_CTL_DEL, pTCPSer->socket, NULL);

err6:
    close(pTCPSer->monitorFd);

err5:
    ThreadPoolDestory(pTCPSer->pThP);

err4:
    HashTableDestory(pTCPSer->pCHTbl);

err3:
    close(pTCPSer->socket);

err2:
    MmMngrFree(pTCPSer);

err1:
    return NULL;
}

VOID_T TcpServerDestory(TCP_SERVER_ST *pTCPSer)
{
    pTCPSer->monitor = 0;
    ThreadPoolDestory(pTCPSer->pThP);
    HashTableDestory(pTCPSer->pCHTbl);
    epoll_ctl(pTCPSer->monitorFd, EPOLL_CTL_DEL, pTCPSer->socket, NULL);
    close(pTCPSer->socket);
    close(pTCPSer->monitorFd);
    MmMngrFree(pTCPSer);
}



