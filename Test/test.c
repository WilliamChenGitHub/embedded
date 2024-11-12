#include "test.h"
#include "Log.h"
#include "ThreadPool.h"
#include "MediaBuffer.h"
#include "Graphics.h"
#include "Thread.h"

const unsigned char F6x8[][6] =		
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,// sp
    0x00, 0x00, 0x00, 0x2f, 0x00, 0x00,// !
    0x00, 0x00, 0x07, 0x00, 0x07, 0x00,// "
    0x00, 0x14, 0x7f, 0x14, 0x7f, 0x14,// #
    0x00, 0x24, 0x2a, 0x7f, 0x2a, 0x12,// $
    0x00, 0x23, 0x13, 0x08, 0x64, 0x62,// %
    0x00, 0x36, 0x49, 0x55, 0x22, 0x50,// &
    0x00, 0x00, 0x05, 0x03, 0x00, 0x00,// '
    0x00, 0x00, 0x1c, 0x22, 0x41, 0x00,// (
    0x00, 0x00, 0x41, 0x22, 0x1c, 0x00,// )
    0x00, 0x14, 0x08, 0x3E, 0x08, 0x14,// *
    0x00, 0x08, 0x08, 0x3E, 0x08, 0x08,// +
    0x00, 0x00, 0x00, 0xA0, 0x60, 0x00,// ,
    0x00, 0x08, 0x08, 0x08, 0x08, 0x08,// -
    0x00, 0x00, 0x60, 0x60, 0x00, 0x00,// .
    0x00, 0x20, 0x10, 0x08, 0x04, 0x02,// /
    0x00, 0x3E, 0x51, 0x49, 0x45, 0x3E,// 0
    0x00, 0x00, 0x42, 0x7F, 0x40, 0x00,// 1
    0x00, 0x42, 0x61, 0x51, 0x49, 0x46,// 2
    0x00, 0x21, 0x41, 0x45, 0x4B, 0x31,// 3
    0x00, 0x18, 0x14, 0x12, 0x7F, 0x10,// 4
    0x00, 0x27, 0x45, 0x45, 0x45, 0x39,// 5
    0x00, 0x3C, 0x4A, 0x49, 0x49, 0x30,// 6
    0x00, 0x01, 0x71, 0x09, 0x05, 0x03,// 7
    0x00, 0x36, 0x49, 0x49, 0x49, 0x36,// 8
    0x00, 0x06, 0x49, 0x49, 0x29, 0x1E,// 9
    0x00, 0x00, 0x36, 0x36, 0x00, 0x00,// :
    0x00, 0x00, 0x56, 0x36, 0x00, 0x00,// ;
    0x00, 0x08, 0x14, 0x22, 0x41, 0x00,// <
    0x00, 0x14, 0x14, 0x14, 0x14, 0x14,// =
    0x00, 0x00, 0x41, 0x22, 0x14, 0x08,// >
    0x00, 0x02, 0x01, 0x51, 0x09, 0x06,// ?
    0x00, 0x32, 0x49, 0x59, 0x51, 0x3E,// @
    0x00, 0x7C, 0x12, 0x11, 0x12, 0x7C,// A
    0x00, 0x7F, 0x49, 0x49, 0x49, 0x36,// B
    0x00, 0x3E, 0x41, 0x41, 0x41, 0x22,// C
    0x00, 0x7F, 0x41, 0x41, 0x22, 0x1C,// D
    0x00, 0x7F, 0x49, 0x49, 0x49, 0x41,// E
    0x00, 0x7F, 0x09, 0x09, 0x09, 0x01,// F
    0x00, 0x3E, 0x41, 0x49, 0x49, 0x7A,// G
    0x00, 0x7F, 0x08, 0x08, 0x08, 0x7F,// H
    0x00, 0x00, 0x41, 0x7F, 0x41, 0x00,// I
    0x00, 0x20, 0x40, 0x41, 0x3F, 0x01,// J
    0x00, 0x7F, 0x08, 0x14, 0x22, 0x41,// K
    0x00, 0x7F, 0x40, 0x40, 0x40, 0x40,// L
    0x00, 0x7F, 0x02, 0x0C, 0x02, 0x7F,// M
    0x00, 0x7F, 0x04, 0x08, 0x10, 0x7F,// N
    0x00, 0x3E, 0x41, 0x41, 0x41, 0x3E,// O
    0x00, 0x7F, 0x09, 0x09, 0x09, 0x06,// P
    0x00, 0x3E, 0x41, 0x51, 0x21, 0x5E,// Q
    0x00, 0x7F, 0x09, 0x19, 0x29, 0x46,// R
    0x00, 0x46, 0x49, 0x49, 0x49, 0x31,// S
    0x00, 0x01, 0x01, 0x7F, 0x01, 0x01,// T
    0x00, 0x3F, 0x40, 0x40, 0x40, 0x3F,// U
    0x00, 0x1F, 0x20, 0x40, 0x20, 0x1F,// V
    0x00, 0x3F, 0x40, 0x38, 0x40, 0x3F,// W
    0x00, 0x63, 0x14, 0x08, 0x14, 0x63,// X
    0x00, 0x07, 0x08, 0x70, 0x08, 0x07,// Y
    0x00, 0x61, 0x51, 0x49, 0x45, 0x43,// Z
    0x00, 0x00, 0x7F, 0x41, 0x41, 0x00,// [
    0x00, 0x55, 0x2A, 0x55, 0x2A, 0x55,// 55
    0x00, 0x00, 0x41, 0x41, 0x7F, 0x00,// ]
    0x00, 0x04, 0x02, 0x01, 0x02, 0x04,// ^
    0x00, 0x40, 0x40, 0x40, 0x40, 0x40,// _
    0x00, 0x00, 0x01, 0x02, 0x04, 0x00,// '
    0x00, 0x20, 0x54, 0x54, 0x54, 0x78,// a
    0x00, 0x7F, 0x48, 0x44, 0x44, 0x38,// b
    0x00, 0x38, 0x44, 0x44, 0x44, 0x20,// c
    0x00, 0x38, 0x44, 0x44, 0x48, 0x7F,// d
    0x00, 0x38, 0x54, 0x54, 0x54, 0x18,// e
    0x00, 0x08, 0x7E, 0x09, 0x01, 0x02,// f
    0x00, 0x18, 0xA4, 0xA4, 0xA4, 0x7C,// g
    0x00, 0x7F, 0x08, 0x04, 0x04, 0x78,// h
    0x00, 0x00, 0x44, 0x7D, 0x40, 0x00,// i
    0x00, 0x40, 0x80, 0x84, 0x7D, 0x00,// j
    0x00, 0x7F, 0x10, 0x28, 0x44, 0x00,// k
    0x00, 0x00, 0x41, 0x7F, 0x40, 0x00,// l
    0x00, 0x7C, 0x04, 0x18, 0x04, 0x78,// m
    0x00, 0x7C, 0x08, 0x04, 0x04, 0x78,// n
    0x00, 0x38, 0x44, 0x44, 0x44, 0x38,// o
    0x00, 0xFC, 0x24, 0x24, 0x24, 0x18,// p
    0x00, 0x18, 0x24, 0x24, 0x18, 0xFC,// q
    0x00, 0x7C, 0x08, 0x04, 0x04, 0x08,// r
    0x00, 0x48, 0x54, 0x54, 0x54, 0x20,// s
    0x00, 0x04, 0x3F, 0x44, 0x40, 0x20,// t
    0x00, 0x3C, 0x40, 0x40, 0x20, 0x7C,// u
    0x00, 0x1C, 0x20, 0x40, 0x20, 0x1C,// v
    0x00, 0x3C, 0x40, 0x30, 0x40, 0x3C,// w
    0x00, 0x44, 0x28, 0x10, 0x28, 0x44,// x
    0x00, 0x1C, 0xA0, 0xA0, 0xA0, 0x7C,// y
    0x00, 0x44, 0x64, 0x54, 0x4C, 0x44,// z
    0x14, 0x14, 0x14, 0x14, 0x14, 0x14,// horiz lines
};

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

static int listNodeCmp(LIST_ST *pListNode1, LIST_ST *pListNode2)
{
    LIST_TST_ST *p1 = ST_CONTAINER_OF(pListNode1, LIST_TST_ST, list);
    LIST_TST_ST *p2 = ST_CONTAINER_OF(pListNode2, LIST_TST_ST, list);

    if(p1->idx > p2->idx)
    {
        return 1;
    }
    
    return 0;
}

void ListTst(void)
{
    LIST_ST list;
    LIST_TST_ST *pNode = NULL, *pN = NULL;
    ListInit(&list);

    for(int32_t i = 0; i < 20; i++)
    {
        LIST_TST_ST *pNew = MmMngrMalloc(sizeof(LIST_TST_ST));
        pNew->idx = GetMonotonicTickNs() / 1000000 % 100;
        LIST_INSERT_BACK(&list, &pNew->list);
        ThreadUsleep(2000);
    }

    LIST_FOREACH_FROM_HEAD(pNode, &list, list)
    {
        LOGS("list idx = %d\r\n", pNode->idx);
    }
    LOGI("*******************\r\n");

    LIST_TST_ST *p1, *p2;
    
    p1 = LIST_NEXT_ENTRY(&list, LIST_TST_ST, list);
    p2 = LIST_PREV_ENTRY(&list, LIST_TST_ST, list);
    LIST_SWAP(&p1->list, &p2->list);

    p1 = LIST_NEXT_ENTRY(&list, LIST_TST_ST, list);
    p2 = LIST_PREV_ENTRY(&list, LIST_TST_ST, list);
    LIST_SWAP(&p1->list, &p2->list);


    p1 = LIST_NEXT_ENTRY(&list, LIST_TST_ST, list);
    p2 = LIST_NEXT_ENTRY(&p2->list, LIST_TST_ST, list);
    LIST_SWAP(&p1->list, &p2->list);

    p1 = LIST_NEXT_ENTRY(&list, LIST_TST_ST, list);
    p2 = LIST_NEXT_ENTRY(&p2->list, LIST_TST_ST, list);
    LIST_SWAP(&p1->list, &p2->list);


    LIST_FOREACH_FROM_HEAD(pNode, &list, list)
    {
        LOGS("list idx = %d\r\n", pNode->idx);
    }
    LOGI("LIST SWAP test done\r\n");
    LOGI("*******************\r\n");

    
    ListSort(&list, listNodeCmp);

    LIST_FOREACH_FROM_HEAD(pNode, &list, list)
    {
        LOGS("list idx = %d\r\n", pNode->idx);
    }
    LOGI("LIST sort test done\r\n");

    LIST_FOREACH_FROM_HEAD_SAFE(pNode, &list, pN, list)
    {
        LIST_DELETE(&pNode->list);
        LOGS("delet %d\r\n", pNode->idx);
        MmMngrFree(pNode);
    }
}

void mission(void *pArg)
{
    int32_t *pI = (int32_t *) pArg;
    LOGI("mission = %d\r\n", *pI);
    LOGI("mission done = %d\r\n", *pI);
}


void ThreadPoolTst(void)
{
    static int32_t i = 0, iarr[100] = {0};
    THREAD_POOL_ST *pTp = ThreadPoolCreate(10, 1024 * 3);

    for(i = 0; i < 100; i++)
    {
        iarr[i] = i;
        ThreadPoolDispatchMission(pTp, mission, &iarr[i]);
    }
    LOGI("dispatch done\r\n");
    ThreadPoolDestory(pTp);
}

static void *threadTest(void *pArg)
{
    (void) pArg;
    LOGI("Thread test \r\n");
    return NULL;
}


static void mediaBufTest(void)
{
    char *testStr1 = "puma are large cat like animal which are found in American, when reports cam into london zoo than a wild puma had been spottd forty five miles south of London";
    
    QUEUE_BUF_LIST_ST bufList[3] = {0};
    
    MEDIA_BUFFER_ST *pMediaBuf = MediaBufCreate(1024 * 1024 * 16);
    if(!pMediaBuf)
    {
        LOGE("media buff create failed\r\n");
        goto err1;
    }

    int len1 = 22;

    LOGI("total len = %d\r\n", strlen(testStr1));
    
    bufList[0].pBuf = testStr1;
    bufList[0].sz = len1;
    bufList[1].pBuf = &testStr1[len1];
    bufList[1].sz = strlen(testStr1) - len1;
    
    for(int i = 0; i  < 1000; i++)
    {
        if(MediaBufPushBufList(pMediaBuf, bufList, bufList[0].sz + bufList[1].sz))
        {
            LOGE("push failed\r\n");
            goto err2;
        }
    }

    char buf[1024] = {0};
    
    for(int i = 0; i  < 1000; i++)
    {
        if(MediaBufPop(pMediaBuf, buf))
        {
            LOGE("pop failed\r\n");
            goto err2;
        }
        
        //LOGI("pop = %s\r\n", buf);

        if(strcmp(buf, testStr1))
        {
            LOGE("cmp %d err = %s\r\n", i, buf);
        }
    }

    for(int i = 0; i  < 1000; i++)
    {
        if(MediaBufPush(pMediaBuf, testStr1, strlen(testStr1)))
        {
            LOGE("222 push failed\r\n");
            goto err2;
        }
    }

    
    for(int i = 0; i  < 1000; i++)
    {
        if(MediaBufPop(pMediaBuf, buf))
        {
            LOGE("222 pop failed\r\n");
            goto err2;
        }
        
        //LOGI("222 pop = %s\r\n", buf);

        if(strcmp(buf, testStr1))
        {
            LOGE("222 cmp %d err = %s\r\n", i, buf);
        }
    }
    
    MediaBufDestroy(pMediaBuf);

    return;
err2:
    MediaBufDestroy(pMediaBuf);

err1:
    return ;
}


static void canvasShow(CANVAS_ATTR_ST *pCanvas)
{

    for(int y = 0; y < pCanvas->h; y++)
    {
        LOGS("\r\n");
        for(int x = 0; x < pCanvas->w; x++)
        {
            if(pCanvas->pixSz == 4)
            {
                GRAPHICS_PIX_UT *p = (GRAPHICS_PIX_UT *)&pCanvas->canvas[pCanvas->lineSz * y + x * pCanvas->pixSz];

                if(p->argb.a)
                {
                    LOGS(LOG_RED"."LOG_CLR_ATTR);
                }
                else if(p->argb.r)
                {
                    LOGS(LOG_GREEN"."LOG_CLR_ATTR);
                }
                else if(p->argb.g)
                {
                    LOGS(LOG_BLUE"."LOG_CLR_ATTR);
                }
                else if(p->argb.b)
                {
                    LOGS(" ");
                }
                else
                {
                    LOGS(" ");
                }
            }
        }
    }
    
    LOGS("\r\n");
}


static void canvasPutC(CANVAS_ATTR_ST *pCanvas, GRAPHICS_POINT_ST *pt, char ch, GRAPHICS_PIX_UT pixColor)
{
    int i,j;
    GRAPHICS_POINT_ST p = {0};

    for(i = 0; i < 6; i++)
    {
        for(j = 0; j < 8; j++)
        {
            if((F6x8[(ch - ' ')][i] >> j) & 0x01)
            {
                p.x = pt->x + i;
                p.y = pt->y + j;
                
                CanvasDrawPoint(pCanvas, &p, pixColor);
            }
        }
    }
}

static void canvasPutS(CANVAS_ATTR_ST *pCanvas, GRAPHICS_POINT_ST *pt, char *string, GRAPHICS_PIX_UT pixColor)
{
    char ch = 0;
    int len = strlen(string);
    GRAPHICS_POINT_ST p = {0};

    p.x = pt->x;
    p.y = pt->y;
    
    for(int i = 0; i < len; i++)
    {
        ch = string[i];
        canvasPutC(pCanvas, &p, ch, pixColor);
        p.x += 6;
    }
}


static void canvasTest(void)
{
    #define CANVAS_SZ_X 64
    #define CANVAS_SZ_Y 32
    
    GRAPHICS_PIX_UT clrColor = {0};
    GRAPHICS_PIX_UT paintColor = {0};
    GRAPHICS_LINE_ST line = {0};
    GRAPHICS_POINT_ST pt = {0};

    paintColor.argb.a = 0;
    paintColor.argb.r = 0xff;
    paintColor.argb.g = 0;
    paintColor.argb.b = 0;
    
    CANVAS_ATTR_ST *pCanvas = CanvasCreate(CANVAS_SZ_X, CANVAS_SZ_Y, GRAPHICS_COLOR_FMT_RGBA8888);

    CanvasClear(pCanvas, clrColor);

    line.p1.x = 0;
    line.p1.y = 0;
    line.p2.x = CANVAS_SZ_X - 1;
    line.p2.y = CANVAS_SZ_Y - 1;
    CanvasDrawLine(pCanvas, &line, paintColor);

    line.p1.x = 0;
    line.p1.y = CANVAS_SZ_Y - 1;
    
    line.p2.x = CANVAS_SZ_X - 1;
    line.p2.y = 0;

    CanvasDrawLine(pCanvas, &line, paintColor);

    GRAPHICS_RECT_ST rect = {0};
    rect.pTopLeft.x = 0;
    rect.pTopLeft.y = 0;
    rect.w = CANVAS_SZ_X - 1;
    rect.h = CANVAS_SZ_Y - 1;

    paintColor.argb.a = 0;
    paintColor.argb.r = 0;
    paintColor.argb.g = 0xff;
    paintColor.argb.b = 0;
    CanvasDrawRect(pCanvas, &rect, paintColor);

    pt.x = 10;
    pt.y = 10;
    canvasPutS( pCanvas, &pt, "CANVAS", paintColor);

    canvasShow(pCanvas);

    CanvasDestroy(pCanvas);
}






// message list test
typedef struct
{
    LIST_ST list;
    int msg;
    int8_t data[0];
}MSG_ATTR_ST;

typedef struct
{
    LIST_ST msgList;
    MUTEX_T lock;
    EVENT_T event;
    bool bDestory;
    THREAD_T msgHdlTh;
    int hdlCnt;
    int sndCnt;
}MSG_LIST_ATTR_ST;

MSG_LIST_ATTR_ST gMessageListAttr = {0};

int SendMsg(MSG_LIST_ATTR_ST *pListAttr, int msgType, void *pMsgData, int msgDataSz)
{
    MSG_ATTR_ST *pNewMsg = MmMngrMalloc(sizeof(*pNewMsg) + msgDataSz);

    if(!pNewMsg)
    {
        LOGE("malloc new msg failed\r\n");
        return -1;
    }

    pNewMsg->msg = msgType;
    memcpy(pNewMsg->data, pMsgData, msgDataSz);
    MutexLock(pListAttr->lock);
    LIST_INSERT_FRONT(&pListAttr->msgList, &pNewMsg->list); // insert to msg list
    pListAttr->sndCnt++;
    MutexUnlock(pListAttr->lock);
    
    EventGeneration(pListAttr->event); // notify msg hdl thread

    return 0;
}



static void * MsgHdl(void *pArg)
{
    MSG_LIST_ATTR_ST *pListAttr = (MSG_LIST_ATTR_ST *)pArg;
    MSG_ATTR_ST *pMsgAttr = NULL, *pMsgNext = NULL;

    while (1)
    {
        while(LIST_IS_EMPTY(&pListAttr->msgList) && !pListAttr->bDestory)
        {
            EventTimedWait(pListAttr->event, -1);
        }
        
        MutexLock(pListAttr->lock);
        if(!LIST_IS_EMPTY(&pListAttr->msgList))
        {
            pMsgAttr = LIST_NEXT_ENTRY(&pListAttr->msgList, typeof(*pMsgAttr), list);
            LIST_DELETE(&pMsgAttr->list); // delete from list

            //LOGI("msg = %d\r\n", pMsgAttr->msg);
            
            pListAttr->hdlCnt++;
            MmMngrFree(pMsgAttr);
            
            MutexUnlock(pListAttr->lock);
        }
        else if(pListAttr->bDestory)
        {
            MutexUnlock(pListAttr->lock);
            break;
        }
        else
        {
            MutexUnlock(pListAttr->lock);
        }
    }

    //delet all msg
    MutexLock(pListAttr->lock);
    LIST_FOREACH_FROM_HEAD_SAFE(pMsgAttr, &pListAttr->msgList, pMsgNext, list)
    {
        LIST_DELETE(&pMsgAttr->list); // delete from list
        LOGE("delet msg %d\r\n", pMsgAttr->msg);
        MmMngrFree(pMsgAttr);
    }
    MutexUnlock(pListAttr->lock);
    return NULL;
}


void sndMsgMission(void *pArg)
{
    int msg = (int)pArg;
    SendMsg(&gMessageListAttr, msg, NULL, 0);
}


void MsgListTest(void)
{
    int sndCnt = 0;
    ListInit(&gMessageListAttr.msgList);
    gMessageListAttr.lock = MutexCreate();
    gMessageListAttr.event = EventCreate();

    gMessageListAttr.msgHdlTh = ThreadCreate(MsgHdl, &gMessageListAttr, 1024 * 3, false, 1, "messagehdl");


    THREAD_POOL_ST *pTp = ThreadPoolCreate(10, 1024);

    for(sndCnt = 0; sndCnt < 100000000; sndCnt++)
    {
        ThreadPoolDispatchMission(pTp, sndMsgMission, (void *)sndCnt);
    }

#if 0
    while(ThreadPoolGetRuningMissions(pTp) || ThreadPoolGetWaitingMissions(pTp))
    {
        sleep(1);
    }
#endif

    ThreadPoolDestory(pTp);

    sleep(3);
    gMessageListAttr.bDestory = true;
    EventGeneration(gMessageListAttr.event);
    ThreadJoin(gMessageListAttr.msgHdlTh);
    
    LOGI("msgHdl cnt = %d, sndCnt = %d\r\n", gMessageListAttr.hdlCnt, gMessageListAttr.sndCnt);

    if(gMessageListAttr.hdlCnt != gMessageListAttr.sndCnt)
    {
        LOGE("snd cnt != hdl cnt\r\n");
    }
    
    MutexDestroy(gMessageListAttr.lock);
    EventDestroy(gMessageListAttr.event);
}


int main(int argc, char **argv)
{
    (void) argc;
    (void) argv;
    FILE_LOG_ATTR_ST log;
    FileLOGCreate(&log, stdin, stdout);
    LOGInit((LOG_ATTR_ST *)&log);
    LOGSetLevel(&log.attr, LOG_LEVEL_DBG);

    LOGS("LOGS\r\n");
    
    LOGD("debug log\r\n");
    LOGI("info log\r\n");
    LOGE("error log\r\n");
    LOGW("warning log\r\n");

    MsgListTest();

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
    mediaBufTest();
    canvasTest();


    TCPSerCliTst(argv[1]);

    LOGDeinit(&log.attr);
    FileLOGDestroy(&log);
    return 0;
}



