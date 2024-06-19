#include "test.h"
#include "Log.h"
#include "ThreadPool.h"


static int32_t tcpCliMsgParse(COM_PORT_ATTR_ST *pComPort,  COM_PACK_ST *pPack, int32_t *pExecRet)
{
    int32_t ret = COM_RET_SUCCESS;

    TCP_CLIENT_ST *pTCPCli = ST_CONTAINER_OF(pComPort, TCP_CLIENT_ST, comPort);

    (void) pTCPCli;
    if(OPTION_IS_SETMETHOD(pPack->option)) // set
    {
        switch(pPack->packId)
        {
            default:
            {
                ret = COM_RET_NOT_SUPPORT_PID;
            }break;
        }
    }
    else // get
    {
        switch(pPack->packId)
        {
            default:
            {
                ret = COM_RET_NOT_SUPPORT_PID;
            }break;
        }
    }
    
    if(*pExecRet)
    {
        ret = COM_RET_EXEC_FAILED;
    }

    return ret;
}



static int32_t tcpSerMsgParse(COM_PORT_ATTR_ST *pComPort, COM_PACK_ST *pPack, int32_t *pExecRet)
{
    int32_t ret = COM_RET_SUCCESS;

    CONNECTION_NODE_ST *pConnection = ST_CONTAINER_OF(pComPort, CONNECTION_NODE_ST, comPort);

    (void)pConnection;
    if(OPTION_IS_SETMETHOD(pPack->option)) // set
    {
        switch(pPack->packId)
        {
            case 2:
            {
                int *pDat = (int *)pPack->data;
                LOGI("receive from client data = %d\r\n", *pDat);
            }break;
                
            default:
            {
                ret = COM_RET_NOT_SUPPORT_PID;
            }break;
        }
    }
    else // get
    {
        switch(pPack->packId)
        {
            case 1:
            {
                int i = 12345;
                ComPortRetPack(pComPort, 1, pPack->packCnt + 1, &i, sizeof i);
            }break;

            
            default:
            {
                ret = COM_RET_NOT_SUPPORT_PID;
            }break;
        }
    }
    
    if(*pExecRet)
    {
        ret = COM_RET_EXEC_FAILED;
    }

    return ret;
}



void tcpCliMission(void *pArg)
{
    char *pSerIp = pArg;
    
    TCP_CLIENT_ST *pTCPCli;

    pTCPCli = TcpClientCreate(pSerIp, 1234, tcpCliMsgParse);
    if(!pTCPCli)
    {
        LOGE("creat tcp client failed\r\n");
        return;
    }

    uint64_t dt = 0;
    CommunicationPing(pTCPCli->pCom, 512, &dt);
    LOGI("ping dt = %lld\r\n", dt / 1000000);

    int buf = 0;
    ComPortGetPack(&pTCPCli->comPort, 1, 1000, 2, NULL, 0, &buf, sizeof buf);
    LOGI("get = %d\r\n", buf);
    ComPortSndPack(&pTCPCli->comPort, 2, true, 1000, 2, &buf, sizeof buf);

    TcpClientDestory(pTCPCli);
    LOGW("client exit\r\n");
}


void TCPSerCliTst(char *pSerIp)
{
    if(pSerIp)
    {
        LOGI("start TCP client\r\n");

        int32_t i = 0;
        THREAD_POOL_ST *pTp = ThreadPoolCreate(500, 1024 * 3);
        
        for(i = 0; i < 50; i++)
        {
            ThreadPoolDispatchMission(pTp, tcpCliMission, pSerIp);
        }
        
        ThreadPoolDestory(pTp);
        LOGW("thp exit\r\n");
    }
    else
    {
        LOGI("start TCP server\r\n");
        TCP_SERVER_ST *pTCPSer = TcpServerCreate(NULL, 1234, tcpSerMsgParse, 20, 1024 * 3, 1000, 0);

        if(!pTCPSer)
        {
            LOGE("creat tcp server failed\r\n");
            return;
        }
        getchar();

        TcpServerDestory(pTCPSer);
    }

}


void HashTableTst(void)
{
    HASH_TABLE_ST *pHashTbl = HashTableCreate(2);
    if(!pHashTbl)
    {
        LOGE("hash table create failed\r\n");
        return ;
    }

    for(int i = 0; i < 10; i++)
    {
        HASH_NODE_ST *pNode = (HASH_NODE_ST *)MmMngrMalloc(sizeof *pNode + 4);
        
        int *pInt = (int *)pNode->data;
        *pInt = GetMonotonicTickNs() % 1000;
        pNode->key = i;
        pNode->pNodeDestory = MmMngrFree;
        
        HashTableInsert(pHashTbl, pNode);
    }

    for(int i = 0; i < 10; i++)
    {
        HASH_NODE_ST *pNode = HashTableLookup(pHashTbl, i);
        if(pNode)
        {
            LOGI("key = %d, dat = %d\r\n", pNode->key, *((int *)pNode->data));
        }
    }

    HashTablePrint(pHashTbl);
    
    HashTableDestory(pHashTbl);
}


typedef struct
{
    LIST_ST list;
    int32_t idx;
}LIST_TST_ST;


void ListTst(void)
{
    LIST_ST list;
    LIST_TST_ST *pNode = NULL, *pN = NULL;
    ListInit(&list);

    for(int32_t i = 0; i < 10; i++)
    {
        LIST_TST_ST *pNew = MmMngrMalloc(sizeof(LIST_TST_ST));
        pNew->idx = i;
        LIST_INSERT_BACK(&list, pNew);
    }

    LIST_FOREACH_FROM_HEAD(pNode, &list)
    {
        LOGS("list idx = %d,", pNode->idx);
    }
    LOGI("LIST_FOREACH_FROM_HEAD\r\n");

    LIST_FOREACH_FROM_TAIL(pNode, &list)
    {
        LOGS("list idx = %d,", pNode->idx);
    }
    LOGI("LIST_FOREACH_FROM_TAIL\r\n");
    

    LIST_TST_ST *p1, *p2;
    
    p1 = LIST_NEXT_ENTRY(&list);
    p2 = LIST_PREV_ENTRY(&list);
    LIST_SWAP(p1, p2);

    p1 = LIST_NEXT_ENTRY(&list);
    p2 = LIST_PREV_ENTRY(&list);
    LIST_SWAP(p1, p2);


    p1 = LIST_NEXT_ENTRY(&list);
    p2 = LIST_NEXT_ENTRY(p2);
    LIST_SWAP(p1, p2);

    p1 = LIST_NEXT_ENTRY(&list);
    p2 = LIST_NEXT_ENTRY(p2);
    LIST_SWAP(p2, p1);

    LIST_FOREACH_FROM_HEAD(pNode, &list)
    {
        LOGS("list idx = %d,", pNode->idx);
    }
    LOGI("LIST_FOREACH_FROM_HEAD\r\n");

    LIST_FOREACH_FROM_HEAD_SAFE(pNode, &list, pN)
    {
        LIST_DELETE(pNode);
        LOGS("delet %d\r\n", pNode->idx);
        MmMngrFree(pNode);
    }
    
}

void mission(void *pArg)
{
    int32_t *pI = (int32_t *) pArg;
    LOGI("arg = %d\r\n", *pI);
}


void ThreadPoolTst(void)
{
    static int32_t i = 0, iarr[100] = {0};
    THREAD_POOL_ST *pTp = ThreadPoolCreate(100, 1024 * 3);

    for(i = 0; i < 10; i++)
    {
        iarr[i] = i;
        ThreadPoolDispatchMission(pTp, mission, &iarr[i]);
    }

    ThreadPoolDestory(pTp);
}

static void *threadTest(void *pArg)
{
    (void) pArg;
    LOGI("Thread test \r\n");
    return NULL;
}


int main(int argc, char **argv)
{
    (void) argc;
    (void) argv;
    FILE_LOG_ATTR_ST log;
    FileLOGCreate((LOG_ATTR_ST *)&log, stdin, stdout);
    LOGInit((LOG_ATTR_ST *)&log);
    LOGSetLevel(LOG_LEVEL_DBG);

    LOGS("LOGS\r\n");
    
    LOGD("debug log\r\n");
    LOGI("info log\r\n");
    LOGE("error log\r\n");
    LOGW("warning log\r\n");

    int32_t i = 0;
    for(i = 0; i < 10; i++)
    {
        LOGI("for1 = %d\r\n", i);
        if(i == 5)
            goto for2;
    }

    for(i = 0; i >=0; i--)
    {
        for2:
            LOGI("for2 = %d\r\n", i);
    }

    ListTst();

    ThreadPoolTst();

    THREAD_T thread = ThreadCreate(threadTest, NULL, 1024, false, 1, "aaa");
    ThreadUsleep(1000);
    ThreadJoin(thread);

    HashTableTst();


    TCPSerCliTst(argv[1]);

    LOGDeinit();
    return 0;
}



