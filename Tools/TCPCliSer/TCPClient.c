#include "TCPClient.h"
#include "MmMngr.h"
#include "stdio.h"

static BOOL_T isIPAddr(const S8_T *server)
{
    S32_T ip[4] = {-1, -1, -1, -1};
    
    sscanf(server, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]);

    if(ip[0] > 255 || ip[0] < 0 || ip[1] > 255 || ip[1] < 0 || ip[2] >255 || ip[2] < 0 || ip[3] > 255 || ip[3] < 0) // is not ip addr
    {
        return FALSE;
    }
    
    return TRUE;
}




TCP_CLIENT_ST *TcpClientCreate(const S8_T *server, const U16_T port, COM_PACK_PARSE_FT pParseCb)
{
    TCP_TRANSFER_FACTORY_ST tcpTransFact;
    TCP_CLIENT_ST *pTCPCli = NULL;
    U64_T on = 1;

    pTCPCli = MmMngrMalloc(sizeof *pTCPCli);
    if(!pTCPCli)
    {
        LOGE("Malloc TCP client struct failed\r\n");
        goto err1;
    }


    pTCPCli->socket = socket(AF_INET, SOCK_STREAM, 0); // create socket
    if(-1 == pTCPCli->socket)
    {
        LOGE("create socket error: %s\r\n", strerror(errno));
        goto err2;
    }

    setsockopt(pTCPCli->socket, SOL_SOCKET, SO_REUSEPORT, (void *)&on, sizeof(on));
    setsockopt(pTCPCli->socket, SOL_SOCKET, SO_REUSEADDR, (void *)&on, sizeof(on));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    
    addr.sin_port = htons(port);//把主机端口号转为网络端口号

    if(isIPAddr(server))
    {
        addr.sin_addr.s_addr = inet_addr(server);
        if(connect(pTCPCli->socket, (struct sockaddr *)&addr, sizeof(addr)))
        {
            LOGE("connect err: %s\r\n", strerror(errno));
            goto err3;
        }
    }
    else
    {
        struct hostent hostbuf, *result;
        char buffer[1024];
        int h_errnop;

        // 使用gethostbyname_r获取主机信息
        int ret = gethostbyname_r(server, &hostbuf, buffer, sizeof(buffer), &result, &h_errnop);

        if (ret == 0 && result != NULL) 
        {
            LOGI("Official name: %s\n", result->h_name);

            // 打印IP地址列表
            LOGI("IP Addresses:\n");
            for (int i = 0; result->h_addr_list[i] != NULL; i++)
            {
                struct in_addr ip;
                memcpy(&ip, result->h_addr_list[i], sizeof(struct in_addr));
                LOGI("ip = %s\n", inet_ntoa(ip));

                addr.sin_addr.s_addr = inet_addr(inet_ntoa(ip));
                if(0 == connect(pTCPCli->socket, (struct sockaddr *)&addr, sizeof(addr)))
                {
                    break;
                }
                else
                {
                    LOGE("connect err: %s\r\n", strerror(errno));
                }
            }
            goto err3;
        }
        else
        {
            LOGE("gethostbyname_r");
            goto err3;
        }
    }



    S32_T flags = fcntl(pTCPCli->socket, F_GETFL, 0);
    if (flags == -1)
    {
        LOGE("Fcntl get failed\r\n");
        goto err3;
    }

    if(fcntl(pTCPCli->socket, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        LOGE("Fcntl set failed\r\n");
        goto err3;
    }

    TCPTransferFactoryInit((TRANSFER_FACTORY_ST *)&tcpTransFact, pTCPCli->socket, true);
    pTCPCli->pCom = CommunicationInit((TRANSFER_FACTORY_ST *)&tcpTransFact, true, true);

    if(!pTCPCli->pCom)
    {
        LOGE("TCP client com create failed\r\n");
        goto err3;
    }

    CommunicationRegPort(pTCPCli->pCom, &pTCPCli->comPort, pParseCb, 1);

    return pTCPCli;


err3:
    close(pTCPCli->socket);

err2:
    MmMngrFree(pTCPCli);

err1:
    return NULL;
}

void TcpClientDestory(TCP_CLIENT_ST *pTCPCli)
{
    CommunicationDeinit(pTCPCli->pCom);
    close(pTCPCli->socket);
    MmMngrFree(pTCPCli);
}



