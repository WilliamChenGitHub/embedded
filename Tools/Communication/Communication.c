#include "Communication.h"
#include "Log.h"

static char *gPackTypeName[] = 
{
    "PACKTP_PING_REQ",
    "PACKTP_PING_RES",
    "PACKTP_THP_REQ", // throughput request
    "PACKTP_THP_DAT",
    "PACKTP_RESET",
    "PACKTP_STOP_SEND",    // stop send
    "PACKTP_CONTINUE_SEND", // continue send
    "PACKTP_HELLO",
    "PACKTP_SYNC",       // sync
    "PACKTP_SYNC_ACK",    // sync ack
    "PACKTP_ACK",
    "PACKTP_RET_DAT",
    "PACKTP_DAT",
    "PACKTP_END",
};


static inline char *GetPackTypeName(COM_PACK_ET packType)
{
    if (PACKTP_END > packType && 0 <= packType)
    {
        return gPackTypeName[packType];
    }
    else
    {
        return "Unknow";
    }
}

// data must be 4 byte alignment
static inline uint32_t CalcSum(uint8_t *data, uint32_t length)
{
    uint32_t sum = 0;
    uint32_t i = 0;
    uint32_t l1 = length & 0xFFFFFFE0;
    uint32_t *p = NULL;

    for (i = 0; i < l1; i += 32)
    {
        p = (uint32_t *)&data[i];
        sum += p[0];
        sum += p[1];
        sum += p[2];
        sum += p[3];
        sum += p[4];
        sum += p[5];
        sum += p[6];
        sum += p[7];
    }

    for( ; i < length; i++)
    {
        sum += data[i];
    }

    return sum;
}

static inline uint16_t CalcPackSum(COM_PACK_ST *pPackHead, uint8_t *pDat, uint16_t dLen)
{
  uint32_t sumOfHead = CalcSum((uint8_t *)&pPackHead->packCnt, PACK_HEAD_SZ - ST_OFFSET_OF(COM_PACK_ST, packCnt)) + pPackHead->len;// 4 byte alignment
  uint32_t sumOfData = CalcSum(pDat, dLen);
  
  uint32_t sum = sumOfHead + sumOfData;
  
  uint16_t *pSum = (uint16_t *)&sum;
  
  return (pSum[0] ^ pSum[1]);
}

static void ClrCommunicationBuf(COM_ATTR_ST *pCom)
{
    TransferClrBuf(pCom->pTransfer);
}

static inline COM_WAIT_PACK_ATTR_ST *GetWaitPackFromHead(COM_ATTR_ST *pCom, uint32_t waitId, uint16_t packId)
{
    COM_WAIT_PACK_ATTR_ST *pWaitPack = NULL, *pRet = NULL;

    MutexLock(pCom->waitPackListMtx);
    LIST_FOREACH_FROM_HEAD(pWaitPack, &pCom->waitPackList, list)
    {
        if(pWaitPack->waitId == waitId && pWaitPack->packId == packId)
        {
            LIST_DELETE(&pWaitPack->list);
            pRet = pWaitPack;
            break;
        }
    }
    MutexUnlock(pCom->waitPackListMtx);
    
    return pRet;
}

static inline COM_WAIT_PACK_ATTR_ST *GetWaitPackFromTail(COM_ATTR_ST *pCom, uint32_t waitId, uint16_t packId)
{
    COM_WAIT_PACK_ATTR_ST *pWaitPack = NULL, *pRet = NULL;

    MutexLock(pCom->waitPackListMtx);
    LIST_FOREACH_FROM_TAIL(pWaitPack, &pCom->waitPackList, list)
    {
        if(pWaitPack->waitId == waitId && pWaitPack->packId == packId)
        {
            LIST_DELETE(&pWaitPack->list);
            pRet = pWaitPack;
            break;
        }
    }
    MutexUnlock(pCom->waitPackListMtx);
    
    return pRet;

}

static inline int32_t AddWaitPack(COM_ATTR_ST *pCom, uint32_t waitId, uint16_t packId, void *rcvBuf, uint16_t bufLen, EVENT_T event, COM_RESULT_ST *pResult)
{
    COM_WAIT_PACK_ATTR_ST *pWaitPack = MmMngrMalloc(sizeof(*pWaitPack));
    
    if(pWaitPack)
    {
        pWaitPack->bufLen = bufLen;
        pWaitPack->waitId = waitId;
        pWaitPack->packId = packId;
        pWaitPack->pBuf = rcvBuf;
        pWaitPack->pResult = pResult;
        pWaitPack->eventWait = event;

        MutexLock(pCom->waitPackListMtx);

        LIST_INSERT_BACK(&pCom->waitPackList, &pWaitPack->list);
        MutexUnlock(pCom->waitPackListMtx);
        return 0;
    }
    
    return -1;
}

static inline int32_t DeletWaitPackWithId(COM_ATTR_ST *pCom, uint32_t waitId, uint16_t packId)
{
    COM_WAIT_PACK_ATTR_ST *pWaitPack = GetWaitPackFromTail(pCom, waitId, packId);

    if(pWaitPack)
    {
        MmMngrFree(pWaitPack);
        return 0;
    }
    return -1;
}


static inline int32_t WaitPack(COM_ATTR_ST *pCom, int32_t wTMs, EVENT_T eventWait, COM_RESULT_ST *pResult)
{
    (void) pCom;
    int32_t ret = EventTimedWait(eventWait, wTMs);

    if(0 > ret)
    {
        if(RESULT_ERR_TIME_OUT == ret)
        {
            LOGE("time out\r\n");
            return COM_RET_TIME_OUT;
        }
        else
        {
            LOGE("sem time wait failed\r\n");
            return COM_RET_WAIT_FAILED;
        }
    }

    return pResult->comRet;
}



static inline COM_RESULT_ST PackTransaction(COM_ATTR_ST *pCom, uint8_t method, uint8_t port, uint32_t ackNb, uint8_t packType, bool isNeedAck, uint32_t wTMs, uint8_t retryCnt, uint64_t tPUs,  uint16_t packId, void *pTxBuf, uint16_t tLen, void *pRxBuf, uint16_t rLen)
{
    COM_RESULT_ST result = {.comRet = COM_RET_NONE};
    TRANSFER_BUF_ST bufList[3] = {0};
    int32_t comRet = COM_RET_SUCCESS;
    EVENT_T eventWait = NULL;

    COM_PACK_ST packHead = {0};
    uint32_t packCnt = 0;

    if(tLen > MAX_PACK_DAT_SZ || rLen > MAX_PACK_DAT_SZ)
    {
        comRet = COM_RET_DATA_EXECEEDED;
        goto exit;
    }
    
    if(pCom->stop)
    {
        comRet = COM_RET_STOPED;
        goto exit;
    }

    MutexLock(pCom->packCntMtx);
    packCnt = ++(pCom->packCnt);
    MutexUnlock(pCom->packCntMtx);
    
    packHead.packCnt = H2COM32(packCnt);
    packHead.port = port;
    
    packHead.sof = H2COM32(COM_SOF);
    packHead.packId = H2COM16(packId);
    packHead.len = H2COM16(tLen);
    
    packHead.option = OPTION_MAKE(isNeedAck, method, packType);
    packHead.ackNb = H2COM32(ackNb);
    packHead.timestamp = H2COM64(tPUs);
    packHead.sumCheck = H2COM16(CalcPackSum(&packHead, pTxBuf, tLen));
    
    if(isNeedAck)
    {
        eventWait = EventCreate();
        if(eventWait)
        {
            if(AddWaitPack(pCom, packCnt + 1, packId, pRxBuf, rLen, eventWait, &result))
            {
                EventDestroy(eventWait);
                comRet = COM_RET_ADD_WAIT_FAILED;
                goto exit;
            }
        }
        else
        {
            comRet = COM_RET_ADD_WAIT_FAILED;
            goto exit;
        }
    }
    
    for(uint8_t cnt = 0; cnt <= retryCnt; cnt++)
    {
        bufList[0].pBuf = &packHead;
        bufList[0].bLen = PACK_HEAD_SZ;
        bufList[1].pBuf = pTxBuf;
        bufList[1].bLen = tLen;
        bufList[2].pBuf = NULL;
        bufList[2].bLen = 0;
        if(0 > TransferTx(pCom->pTransfer, bufList, PACK_HEAD_SZ + tLen))
        {
            comRet = COM_RET_TX_FAILED;
            continue;
        }

        if(isNeedAck)
        {
            comRet = WaitPack(pCom, wTMs, eventWait, &result);

            switch(comRet)
            {
                case COM_RET_NONE:
                case COM_RET_TIME_OUT:
                {
                    continue;
                }break;

                case COM_RET_WAIT_FAILED:
                {
                    continue;
                }break;

                default : // success or failed
                {
                    goto exit2;
                }break;
            }
        }
        else
        {
            comRet = COM_RET_SUCCESS;
            goto exit;
        }
    }

    if(isNeedAck)
    {
        if(DeletWaitPackWithId(pCom, packCnt + 1, packId)) // recheck wether the wait pack is exist if exist delet
        {
            //if the pack does not exist, the pack must be get by the communicationProcThread so we need wait again
            comRet = WaitPack(pCom, wTMs, eventWait, &result);
        }

exit2:
        EventDestroy(eventWait);
    }

exit:
    result.comRet = comRet;

    if(COM_RET_SUCCESS != comRet)
    {
        LOGE("ERROR , port = %d, packType = %s, pack id = %d, com ret = %d, exec ret = %d\r\n", port, GetPackTypeName(packType), packId, result.comRet, result.execRet);
    }
    return result;
}



static inline COM_RESULT_ST TxSyncPack(COM_ATTR_ST *pCom, uint8_t packType, void *pDat, uint16_t len)
{
    return PackTransaction(pCom, OPTION_SETMETHOD, 0, 0, packType, false, 0, 0, GET_SYS_TICK_US(), 0, pDat, len, NULL, 0);
}

COM_RESULT_ST CommunicationSync(COM_ATTR_ST *pCom)
{
    COM_TIME_SYNC_ST timeSync = {0};
    return TxSyncPack(pCom, PACKTP_SYNC, &timeSync, sizeof(timeSync));
}

int32_t CommunicationReset(COM_ATTR_ST *pCom)
{
    int32_t cnt = 0;

    COM_RESULT_ST ret = CommunicationTxPack(pCom, 0, 0, PACKTP_HELLO, true, 1000, 0, 0, NULL, 0); // Is communication well?

    if(COM_RET_SUCCESS != ret.comRet) // if failed to do reset programa
    {
        CommunicationTxPack(pCom, 0, 0, PACKTP_STOP_SEND, false, 0, 0, 0, NULL, 0); // stop
        CommunicationTxPack(pCom, 0, 0, PACKTP_RESET, false, 0, 0, 0, NULL, 0);// reset
        // wait device reset completed
        for(cnt = 0; (cnt < 500) && CalcQueueDataSz(pCom->pTransfer->pTxQueue); cnt++)
        {
            ThreadUsleep(1000);
        }
        ThreadUsleep(1000 * 10);

        CommunicationTxPack(pCom, 0, 0, PACKTP_STOP_SEND, true, 1000, 3, 0, NULL, 0); // stop device send message
        CommunicationTxPack(pCom, 0, 0, PACKTP_RESET, false, 0, 0, 0, NULL, 0);// repeat send ensure that clear device buffer is successful
        CommunicationTxPack(pCom, 0, 0, PACKTP_RESET, false, 0, 0, 0, NULL, 0);// repeat send ensure that clear device buffer is successful

        // wait device reset completed
        for(cnt = 0; (cnt < 500) && CalcQueueDataSz(pCom->pTransfer->pTxQueue); cnt++)
        {
            ThreadUsleep(1000);
        }
        ThreadUsleep(1000 * 10);

        CommunicationTxPack(pCom, 0, 0, PACKTP_CONTINUE_SEND, true, 1000, 3, 0, NULL, 0); // start device send message
        ret = CommunicationTxPack(pCom, 0, 0, PACKTP_HELLO, true, 1000, 3, 0, NULL, 0); //Is communication well?
        return ret.comRet;
    }
    else
    {
        return ret.comRet;
    }
}


static inline COM_RESULT_ST SendAckPack(COM_ATTR_ST *pCom, uint32_t ackNb, uint16_t packId, void *pDat, uint16_t dLen)
{
    return CommunicationTxPack(pCom, 0, ackNb, PACKTP_ACK, false, 0, 0, packId, pDat, dLen);
}

static inline COM_RESULT_ST SendPingRes(COM_ATTR_ST *pCom, uint32_t ackNb, uint8_t *pDat, uint16_t pingDlen)
{
    return CommunicationTxPack(pCom, 0, ackNb, PACKTP_PING_RES, false, 0, 0, 0, pDat, pingDlen);
}


static void *ThpReqTh(void *arg)
{
    COM_THP_REQATTR_ST *pThpAttr = arg;
    COM_ATTR_ST *pCom = *((COM_ATTR_ST **)((uint8_t *)arg + sizeof(COM_THP_REQATTR_ST)));
    
    CommunicationTxThroughput(pCom, pThpAttr->reqDatSz, pThpAttr->packSz);
    
    return NULL;
}

static int32_t RunThpReq(COM_ATTR_ST *pCom, COM_THP_REQATTR_ST *pThpAttr)
{
    THREAD_T thId = 0;
    static uint8_t arg[sizeof(COM_THP_REQATTR_ST) + sizeof(COM_ATTR_ST *)] = {0};
    memcpy(arg, pThpAttr, sizeof(COM_THP_REQATTR_ST));
    memcpy(&arg[sizeof(COM_THP_REQATTR_ST)], &pCom, sizeof(COM_ATTR_ST *));

    thId = ThreadCreate(ThpReqTh, arg, 2 * 1024, true, 10, "ComThpThread");

    if(NULL == thId)
    {
        LOGE("RunThpReq failed\r\n");
        return -1;
    }

    return 0;
}

static void *TimeSyncTh(void *arg)
{
    COM_ATTR_ST *pCom = (COM_ATTR_ST *)arg;
    
    while(!pCom->exit)
    {
        CommunicationSync(pCom);
        EventTimedWait(pCom->timeSyncEvent, -1);
    }

    return NULL;
}

int32_t CommunicaitonStartTimeSync(COM_ATTR_ST *pCom)
{
    EventGeneration(pCom->timeSyncEvent);
    return 0;
}


static void *CommunicationProcThread(void *argument)
{
    COM_ATTR_ST *pCom = (COM_ATTR_ST *)argument;
    COM_PACK_ST *pPack = NULL;
    uint8_t buffer[MAX_PACK_SZ_TOTAL] = {0};
    uint16_t packLen = 0;
    uint16_t sumCheck = 0;
    int32_t reqLen = 0, getlen = 0;
    
    pPack = (COM_PACK_ST *)buffer;

    do
    {
        COM_RESULT_ST ackResult = {0};

        reqLen = ST_OFFSET_OF(COM_PACK_ST, packCnt);
        getlen = TransferReadRxDat(pCom->pTransfer, buffer, reqLen); // get head
        if(reqLen > getlen)
        {
            continue; // time out
        }
        else if(COM_SOF != COM2H32(pPack->sof))
        {
            TransferReleaseRxDat(pCom->pTransfer, sizeof(pPack->sof)); // release sof 
            continue;
        }
        
        TransferReleaseRxDat(pCom->pTransfer, sizeof(pPack->sof)); // release sof
        
        packLen = COM2H16(pPack->len);
        if(packLen & 0x0003)
        {
            LOGE("Error, packe len = %d is not mutiple of 4 or reqLen is zero id = %d, tp = %d\r\n", packLen, COM2H16(pPack->packId), OPTION_PACKTP(pPack->option));
            continue;
        }
        else if(MAX_PACK_DAT_SZ < packLen)
        {
            LOGE("Error, %d < pack->len\r\n", MAX_PACK_DAT_SZ);
            continue;
        }
        
        TransferReleaseRxDat(pCom->pTransfer, reqLen - sizeof(pPack->sof)); // release sumCheck and len
        
        reqLen = PACK_HEAD_SZ + packLen - reqLen;
        getlen = TransferReadRxDat(pCom->pTransfer, &(pPack->packCnt), reqLen); // read data and pack head
        if(reqLen > getlen)
        {
            LOGE("Error, reqRxDat timeout getlen = %d, pack len = %d\r\n", getlen, packLen);
            continue;
        }

        
        if(pCom->exit)
        {
            break;
        }

        sumCheck = CalcPackSum(pPack, pPack->data, packLen);
        pPack->sumCheck = COM2H16(pPack->sumCheck);

        if(sumCheck != pPack->sumCheck)
        {
            LOGE("Error, sum check err, 0x%x != 0x%x\r\n", sumCheck, pPack->sumCheck);
            LOGE("option=%d, packId%d, len%d, packCnt%d, port%d\r\n", pPack->option, pPack->packId, pPack->len, pPack->packCnt, pPack->port);
            continue;
        }

        TransferReleaseRxDat(pCom->pTransfer, reqLen); // success, release a pack size

        pPack->packCnt = COM2H32(pPack->packCnt);
        pPack->packId = COM2H16(pPack->packId);
        pPack->ackNb = COM2H32(pPack->ackNb);

        switch(OPTION_PACKTP(pPack->option))
        {
            case PACKTP_PING_REQ:
            {
                SendPingRes(pCom, pPack->packCnt + 1, pPack->data, packLen);
            }break;
            
            case PACKTP_PING_RES:
            {
                COM_WAIT_PACK_ATTR_ST *pWaitPack = GetWaitPackFromTail(pCom, pPack->ackNb, pPack->packId);
                if(pWaitPack)
                {
                    if(pWaitPack->pBuf)
                    {
                        memcpy(pWaitPack->pBuf, pPack->data, pWaitPack->bufLen);
                    }
                    pWaitPack->pResult->comRet = COM_RET_SUCCESS;
                    EventGeneration(pWaitPack->eventWait);
                    MmMngrFree(pWaitPack);
                }
            }break;
            
            case PACKTP_THP_DAT:
            {
            }break;
            
            case PACKTP_THP_REQ:
            {
                COM_THP_REQATTR_ST *pThpAttr = (COM_THP_REQATTR_ST *)pPack->data;
                pThpAttr->packSz = COM2H16(pThpAttr->packSz);
                pThpAttr->reqDatSz = COM2H32(pThpAttr->reqDatSz);

                if(OPTION_IS_NEEDACK(pPack->option))
                {
                    ackResult.comRet = H2COM32(COM_RET_SUCCESS);
                    SendAckPack(pCom, pPack->packCnt + 1, pPack->packId, &ackResult, sizeof(ackResult));
                }
                RunThpReq(pCom, pThpAttr);
            }break;
            
            case PACKTP_RESET:
            {
                ClrCommunicationBuf(pCom);
                if(OPTION_IS_NEEDACK(pPack->option))
                {
                    int32_t stop = pCom->stop;
                    pCom->stop = 0;
                    ackResult.comRet = H2COM32(COM_RET_SUCCESS);
                    SendAckPack(pCom, pPack->packCnt + 1, pPack->packId, &ackResult, sizeof(ackResult));
                    pCom->stop = stop;
                }
            }break;

            case PACKTP_STOP_SEND:
            {
                if(OPTION_IS_NEEDACK(pPack->option))
                {
                    pCom->stop = 0;
                    ackResult.comRet = H2COM32(COM_RET_SUCCESS);
                    SendAckPack(pCom, pPack->packCnt + 1, pPack->packId, &ackResult, sizeof(ackResult));
                }
                pCom->stop = 1;
            }break;

            case PACKTP_CONTINUE_SEND:
            {
                pCom->stop = 0;
                if(OPTION_IS_NEEDACK(pPack->option))
                {
                    ackResult.comRet = H2COM32(COM_RET_SUCCESS);
                    SendAckPack(pCom, pPack->packCnt + 1, pPack->packId, &ackResult, sizeof(ackResult));
                }
            }break;

            case PACKTP_HELLO:
            {
                if(OPTION_IS_NEEDACK(pPack->option))
                {
                    ackResult.comRet = H2COM32(COM_RET_SUCCESS);
                    SendAckPack(pCom, pPack->packCnt + 1, pPack->packId, &ackResult, sizeof(ackResult));
                }
            }break;
            
            case PACKTP_SYNC: // time sync slave
            {
                COM_TIME_SYNC_ST *pTimeSync = (COM_TIME_SYNC_ST *)pPack->data;
                
                pTimeSync->t2 = H2COM64(GET_SYS_TICK_US()); // fill t2
                pTimeSync->t1 = pPack->timestamp; // rec t1
                
                TxSyncPack(pCom, PACKTP_SYNC_ACK, pTimeSync, sizeof(*pTimeSync));
                
                if(OPTION_IS_NEEDACK(pPack->option))
                {
                    ackResult.comRet = H2COM32(COM_RET_SUCCESS);
                    SendAckPack(pCom, pPack->packCnt + 1, pPack->packId, &ackResult, sizeof(ackResult));
                }
            }break;

            case PACKTP_SYNC_ACK: // time sync master
            {
                COM_TIME_SYNC_ST *pTimeSync = (COM_TIME_SYNC_ST *)pPack->data;
                pTimeSync->t4 = GET_SYS_TICK_US(); // fill t4
                pTimeSync->t3 = COM2H64(pPack->timestamp); // rec t3
                
                pTimeSync->t1 = COM2H64(pTimeSync->t1);
                pTimeSync->t2 = COM2H64(pTimeSync->t2);
                
                pCom->timeOffset = ((pTimeSync->t2 - pTimeSync->t1) + (pTimeSync->t3 - pTimeSync->t4)) >> 1;
                LOGD("time offset = %lld\r\n", pCom->timeOffset);
            }break;


            case PACKTP_RET_DAT:
            {
                COM_WAIT_PACK_ATTR_ST *pWaitPack = GetWaitPackFromTail(pCom, pPack->ackNb, pPack->packId);
                
                if(pWaitPack)
                {
                    if(pWaitPack->pBuf)
                    {
                        memcpy(pWaitPack->pBuf, pPack->data, pWaitPack->bufLen);
                    }
                    pWaitPack->pResult->comRet = COM_RET_SUCCESS;
                    EventGeneration(pWaitPack->eventWait);
                    MmMngrFree(pWaitPack);
                }
            }break;
            
            case PACKTP_ACK:
            {
                COM_WAIT_PACK_ATTR_ST *pWaitPack = GetWaitPackFromTail(pCom, pPack->ackNb, pPack->packId);
                COM_RESULT_ST *pResult = (COM_RESULT_ST *)pPack->data;
                
                if(pWaitPack)
                {
                    pWaitPack->pResult->comRet = COM2H32(pResult->comRet);
                    pWaitPack->pResult->execRet = COM2H32(pResult->execRet);
                    EventGeneration(pWaitPack->eventWait);
                    MmMngrFree(pWaitPack);
                }
            }break;
            
            case PACKTP_DAT:
            {
                COM_PORT_ATTR_ST *pPort = NULL, *pPortMatched = NULL;
                MutexLock(pCom->portListMtx); 
                LIST_FOREACH_FROM_HEAD(pPort, &pCom->portList, list)
                {
                    if(pPort->port == pPack->port)
                    {
                        pPortMatched = pPort;
                        break; // matched
                    }
                }
                MutexUnlock(pCom->portListMtx);

                if(pPortMatched)
                {
                    ackResult.comRet = pPort->pPackParse(pPort, pPack, &ackResult.execRet);
                }
                else
                {
                    ackResult.comRet = COM_RET_PORT_NOT_FOUND;
                }

                if(OPTION_IS_NEEDACK(pPack->option) || (OPTION_IS_GETMETHOD(pPack->option) && COM_RET_SUCCESS != ackResult.comRet))
                {
                    ackResult.comRet = H2COM32(ackResult.comRet);
                    ackResult.execRet = H2COM32(ackResult.execRet);
                    SendAckPack(pCom, pPack->packCnt + 1, pPack->packId, &ackResult, sizeof(ackResult));
                }
            }break;
        }
        LOGD("%s, %d\r\n", GetPackTypeName(OPTION_PACKTP(pPack->option)), OPTION_PACKTP(pPack->option));
    }
    while(!pCom->exit);

    return NULL;
}


COM_ATTR_ST *CommunicationInit(TRANSFER_FACTORY_ST *pTransferFactory, bool isMaster, bool createTh2Parse)
{
    COM_ATTR_ST *pCom = NULL;
    TRANSFER_ST *pTransfer = NULL;
    int32_t ret = 0;
    
    pCom = MmMngrMalloc(sizeof *pCom);
    if(!pCom)
    {
        LOGE("Creat communicaiton attr struct failed\r\n");
        goto err1;
    }
    memset(pCom, 0, sizeof *pCom);

    pCom->isMaster = isMaster;
    ListInit(&pCom->waitPackList);
    ListInit(&pCom->portList);
    
    pTransfer = TransferCreate(pTransferFactory);
    if(!pTransfer)
    {
        LOGE("Creat Transfer failed");
        goto err2;
    }

    pCom->pTransfer = pTransfer;
    
    pCom->portListMtx = MutexCreate();
    if(NULL == pCom->portListMtx)
    {
        LOGE("Creat protlist mutex failed\r\n");
        goto err3;
    }
    
    pCom->packCntMtx = MutexCreate();
    if(NULL == pCom->packCntMtx)
    {
        LOGE("Creat pack cnt mutex failed\r\n");
        goto err4;
    }
    
    pCom->waitPackListMtx = MutexCreate();
    if(NULL == pCom->waitPackListMtx)
    {
        LOGE("Creat wait pack list mutex failed\r\n");
        goto err5;
    }

    if(createTh2Parse)
    {
        pCom->thId = ThreadCreate(CommunicationProcThread, pCom, 3 * 1024, false, 3, "CommunicationProc");

        if(NULL == pCom->thId)
        {
            LOGE("Creat Communication thread failed\r\n");
            goto err6;
        }
    }

    if(isMaster)
    {
        pCom->timeSyncEvent = EventCreate();
        if(NULL ==  pCom->timeSyncEvent)
        {
            LOGE("Create timeSync cond failed\r\n");
            goto err7;
        }

        ret = CommunicationReset(pCom);
        if(ret)
        {
            LOGE("communication reset failed\r\n");
            goto err8;
        }

        pCom->timeSyncthId = ThreadCreate(TimeSyncTh, pCom, 2 * 1024, false, 10, "ComTimeSync");
        if(NULL == pCom->timeSyncthId)
        {
            LOGE("Create time sync thread failed\r\n");
            goto err9;
        }
    }

    return pCom;


err9:

err8:
    EventDestroy(pCom->timeSyncEvent);

err7:
    pCom->exit = 1;
    TransferPrepareDeinit(pCom->pTransfer);

    if(createTh2Parse)
    {
        ThreadJoin(pCom->thId);
    }

err6:
    MutexDestroy(pCom->waitPackListMtx);

err5:
    MutexDestroy(pCom->packCntMtx);

err4:
    MutexDestroy(pCom->portListMtx);
    
err3:
    TransferDestory(pCom->pTransfer);

err2:
    MmMngrFree(pCom);
    
err1:
    return NULL;
}


void CommunicationTryParse(COM_ATTR_ST *pCom)
{
    COM_PACK_ST *pPack = NULL;
    uint8_t buffer[MAX_PACK_SZ_TOTAL] = {0};
    uint16_t packLen = 0;
    uint16_t sumCheck = 0;
    int32_t reqLen = 0, getlen = 0;
    
    pPack = (COM_PACK_ST *)buffer;

    do
    {
        COM_RESULT_ST ackResult = {0};

        reqLen = PACK_HEAD_SZ;
        getlen = TransferReadRxDat(pCom->pTransfer, buffer, reqLen); // get head
        if(reqLen > getlen)
        {
            return ; // data len is not enough
        }
        else if(COM_SOF != COM2H32(pPack->sof))
        {
            TransferReleaseRxDat(pCom->pTransfer, reqLen); // data is illeage, release
            continue;
        }
        
        packLen = COM2H16(pPack->len);
        if(packLen & 0x0003)
        {
            TransferReleaseRxDat(pCom->pTransfer, reqLen); // data is illeage, release
            LOGE("Error, packe len = %d is not mutiple of 4 or reqLen is zero id = %d, tp = %d\r\n", packLen, COM2H16(pPack->packId), OPTION_PACKTP(pPack->option));
            continue;
        }
        else if(MAX_PACK_DAT_SZ < packLen)
        {
            TransferReleaseRxDat(pCom->pTransfer, reqLen); // data is illeage, release
            LOGE("Error, %d < pack->len\r\n", MAX_PACK_DAT_SZ);
            continue;
        }

        reqLen = packLen + PACK_HEAD_SZ;
        getlen = TransferReadRxDat(pCom->pTransfer, pPack, reqLen); // read data
        if(reqLen > getlen)
        {
            return ;// data len is not enough
        }

        sumCheck = CalcPackSum(pPack, pPack->data, packLen);
        pPack->sumCheck = COM2H16(pPack->sumCheck);

        TransferReleaseRxDat(pCom->pTransfer, PACK_HEAD_SZ + packLen); // release

        if(sumCheck != pPack->sumCheck)
        {
            LOGE("Error, sum check err, 0x%x != 0x%x\r\n", sumCheck, pPack->sumCheck);
            LOGE("option=%d, packId%d, len%d, packCnt%d, port%d\r\n", pPack->option, pPack->packId, pPack->len, pPack->packCnt, pPack->port);
            continue;
        }

        pPack->packCnt = COM2H32(pPack->packCnt);
        pPack->packId = COM2H16(pPack->packId);
        pPack->ackNb = COM2H32(pPack->ackNb);

        switch(OPTION_PACKTP(pPack->option))
        {
            case PACKTP_PING_REQ:
            {
                SendPingRes(pCom, pPack->packCnt + 1, pPack->data, packLen);
            }break;
            
            case PACKTP_PING_RES:
            {
                COM_WAIT_PACK_ATTR_ST *pWaitPack = GetWaitPackFromTail(pCom, pPack->ackNb, pPack->packId);
                if(pWaitPack)
                {
                    if(pWaitPack->pBuf)
                    {
                        memcpy(pWaitPack->pBuf, pPack->data, pWaitPack->bufLen);
                    }
                    pWaitPack->pResult->comRet = COM_RET_SUCCESS;
                    EventGeneration(pWaitPack->eventWait);
                    MmMngrFree(pWaitPack);
                }
            }break;
            
            case PACKTP_THP_DAT:
            {
            }break;
            
            case PACKTP_THP_REQ:
            {
                COM_THP_REQATTR_ST *pThpAttr = (COM_THP_REQATTR_ST *)pPack->data;
                pThpAttr->packSz = COM2H16(pThpAttr->packSz);
                pThpAttr->reqDatSz = COM2H32(pThpAttr->reqDatSz);

                if(OPTION_IS_NEEDACK(pPack->option))
                {
                    ackResult.comRet = H2COM32(COM_RET_SUCCESS);
                    SendAckPack(pCom, pPack->packCnt + 1, pPack->packId, &ackResult, sizeof(ackResult));
                }
                RunThpReq(pCom, pThpAttr);
            }break;
            
            case PACKTP_RESET:
            {
                ClrCommunicationBuf(pCom);
                if(OPTION_IS_NEEDACK(pPack->option))
                {
                    int32_t stop = pCom->stop;
                    pCom->stop = 0;
                    ackResult.comRet = H2COM32(COM_RET_SUCCESS);
                    SendAckPack(pCom, pPack->packCnt + 1, pPack->packId, &ackResult, sizeof(ackResult));
                    pCom->stop = stop;
                }
            }break;

            case PACKTP_STOP_SEND:
            {
                if(OPTION_IS_NEEDACK(pPack->option))
                {
                    pCom->stop = 0;
                    ackResult.comRet = H2COM32(COM_RET_SUCCESS);
                    SendAckPack(pCom, pPack->packCnt + 1, pPack->packId, &ackResult, sizeof(ackResult));
                }
                pCom->stop = 1;
            }break;

            case PACKTP_CONTINUE_SEND:
            {
                pCom->stop = 0;
                if(OPTION_IS_NEEDACK(pPack->option))
                {
                    ackResult.comRet = H2COM32(COM_RET_SUCCESS);
                    SendAckPack(pCom, pPack->packCnt + 1, pPack->packId, &ackResult, sizeof(ackResult));
                }
            }break;

            case PACKTP_HELLO:
            {
                if(OPTION_IS_NEEDACK(pPack->option))
                {
                    ackResult.comRet = H2COM32(COM_RET_SUCCESS);
                    SendAckPack(pCom, pPack->packCnt + 1, pPack->packId, &ackResult, sizeof(ackResult));
                }
            }break;
            
            case PACKTP_SYNC: // time sync slave
            {
                COM_TIME_SYNC_ST *pTimeSync = (COM_TIME_SYNC_ST *)pPack->data;
                
                pTimeSync->t2 = H2COM64(GET_SYS_TICK_US()); // fill t2
                pTimeSync->t1 = pPack->timestamp; // rec t1
                
                TxSyncPack(pCom, PACKTP_SYNC_ACK, pTimeSync, sizeof(*pTimeSync));
                
                if(OPTION_IS_NEEDACK(pPack->option))
                {
                    ackResult.comRet = H2COM32(COM_RET_SUCCESS);
                    SendAckPack(pCom, pPack->packCnt + 1, pPack->packId, &ackResult, sizeof(ackResult));
                }
            }break;

            case PACKTP_SYNC_ACK: // time sync master
            {
                COM_TIME_SYNC_ST *pTimeSync = (COM_TIME_SYNC_ST *)pPack->data;
                pTimeSync->t4 = GET_SYS_TICK_US(); // fill t4
                pTimeSync->t3 = COM2H64(pPack->timestamp); // rec t3
                
                pTimeSync->t1 = COM2H64(pTimeSync->t1);
                pTimeSync->t2 = COM2H64(pTimeSync->t2);
                
                pCom->timeOffset = ((pTimeSync->t2 - pTimeSync->t1) + (pTimeSync->t3 - pTimeSync->t4)) >> 1;
                LOGD("time offset = %lld\r\n", pCom->timeOffset);
            }break;


            case PACKTP_RET_DAT:
            {
                COM_WAIT_PACK_ATTR_ST *pWaitPack = GetWaitPackFromTail(pCom, pPack->ackNb, pPack->packId);
                
                if(pWaitPack)
                {
                    if(pWaitPack->pBuf)
                    {
                        memcpy(pWaitPack->pBuf, pPack->data, pWaitPack->bufLen);
                    }
                    pWaitPack->pResult->comRet = COM_RET_SUCCESS;
                    EventGeneration(pWaitPack->eventWait);
                    MmMngrFree(pWaitPack);
                }
            }break;
            
            case PACKTP_ACK:
            {
                COM_WAIT_PACK_ATTR_ST *pWaitPack = GetWaitPackFromTail(pCom, pPack->ackNb, pPack->packId);
                COM_RESULT_ST *pResult = (COM_RESULT_ST *)pPack->data;
                
                if(pWaitPack)
                {
                    pWaitPack->pResult->comRet = COM2H32(pResult->comRet);
                    pWaitPack->pResult->execRet = COM2H32(pResult->execRet);
                    EventGeneration(pWaitPack->eventWait);
                    MmMngrFree(pWaitPack);
                }
            }break;
            
            
            case PACKTP_DAT:
            {
                COM_PORT_ATTR_ST *pPort = NULL, *pPortMatched = NULL;
                MutexLock(pCom->portListMtx); 
                LIST_FOREACH_FROM_HEAD(pPort, &pCom->portList, list)
                {
                    if(pPort->port == pPack->port)
                    {
                        pPortMatched = pPort;
                        break; // matched
                    }
                }
                MutexUnlock(pCom->portListMtx);

                if(pPortMatched)
                {
                    ackResult.comRet = pPort->pPackParse(pPort, pPack, &ackResult.execRet);
                }
                else
                {
                    ackResult.comRet = COM_RET_PORT_NOT_FOUND;
                }

                if(OPTION_IS_NEEDACK(pPack->option) || (OPTION_IS_GETMETHOD(pPack->option) && COM_RET_SUCCESS != ackResult.comRet))
                {
                    ackResult.comRet = H2COM32(ackResult.comRet);
                    ackResult.execRet = H2COM32(ackResult.execRet);
                    SendAckPack(pCom, pPack->packCnt + 1, pPack->packId, &ackResult, sizeof(ackResult));
                }
            }break;
        }
        LOGD("%s, %d\r\n", GetPackTypeName(OPTION_PACKTP(pPack->option)), OPTION_PACKTP(pPack->option));
    }
    while(!pCom->exit);
}


void CommunicationDeinit(COM_ATTR_ST *pCom)
{
    pCom->exit = 1;

    if(pCom->isMaster)
    {
        // exit time sync thread
        EventGeneration(pCom->timeSyncEvent);
        ThreadJoin(pCom->timeSyncthId);
        EventDestroy(pCom->timeSyncEvent);
    }

    TransferPrepareDeinit(pCom->pTransfer);

    if(pCom->thId)
    {
        ThreadJoin(pCom->thId);
    }

    TransferDestory(pCom->pTransfer);
    
    MutexDestroy(pCom->packCntMtx);
    MutexDestroy(pCom->portListMtx);
    
    MutexDestroy(pCom->waitPackListMtx);

    MmMngrFree(pCom);
}



// port can not be zero
// pCom: which communication object, pComPort: communicaiton port object, pFnPackParse: communicaiton port pack process call back function, port: port number
int32_t CommunicationRegPort(COM_ATTR_ST *pCom, COM_PORT_ATTR_ST *pComPort, COM_PACK_PARSE_FT pFnPackParse, uint8_t port)
{
    if(!pCom || !pComPort)
    {
        return -1;
    }
    
    pComPort->port = port;
    pComPort->pCom = pCom;
    pComPort->pPackParse = pFnPackParse;

    MutexLock(pCom->portListMtx);
    LIST_INSERT_BACK(&pCom->portList, &pComPort->list);
    MutexUnlock(pCom->portListMtx);

    return 0;
}

void CommunicationUnregPort(COM_PORT_ATTR_ST *pComPort)
{
    if(!pComPort || !pComPort->pCom)
    {
        return;
    }
    
    MutexLock(pComPort->pCom->portListMtx);
    LIST_DELETE(&pComPort->list);
    MutexUnlock(pComPort->pCom->portListMtx);
}




// send a pack
// pCom: communication object, packId: pack ID, ackNb: if is ack pack , ackNb is require, ackNb equal pack count + 1. packType: reference COM_PACK_ET 
//port: port number, isNeedAck: is ack pack require, wTMs: wait time(ms), pDat: paylod, len: payload len 

COM_RESULT_ST CommunicationTxPack(COM_ATTR_ST *pCom, uint8_t port, uint32_t ackNb, uint8_t packType, bool isNeedAck, uint32_t wTMs, uint8_t retryCnt, uint16_t packId, void *pDat, uint16_t len)
{
    return PackTransaction(pCom, OPTION_SETMETHOD, port, ackNb, packType, isNeedAck, wTMs, retryCnt, COM_GET_TICK_US(pCom), packId, pDat, len, NULL, 0);
}

// get a pack
// pCom: communication object, packId: pack ID, ackNb: if is ack pack , packType: reference COM_PACK_ET 
//port: port number,  wTMs: wait time(ms), pBuf: paylod buffer, len: payload buffer len 
COM_RESULT_ST CommunicationGetPack(COM_ATTR_ST *pCom, uint8_t port, uint8_t packType, uint32_t wTMs, uint8_t retryCnt, uint16_t packId, void *pTxDat, uint16_t tLen, void *pRxBuf, uint16_t bLen)
{
    return PackTransaction(pCom, OPTION_GETMETHOD, port, 0, packType, true, wTMs, retryCnt, COM_GET_TICK_US(pCom), packId, pTxDat, tLen, pRxBuf, bLen);
}


int32_t ComPortRetPack(COM_PORT_ATTR_ST *pComPort, uint16_t packId, uint32_t ackNb, void *pDat, uint16_t dLen)
{
    COM_RESULT_ST result = CommunicationTxPack(pComPort->pCom, pComPort->port, ackNb, PACKTP_RET_DAT, false, 0, 0, packId, pDat, dLen);
    return result.comRet;
}

int32_t ComPortSndPack(COM_PORT_ATTR_ST *pComPort, uint16_t packId, bool isNeedAck, uint32_t wTMs, uint8_t retryCnt, void *pDat, uint16_t dLen)
{
    COM_RESULT_ST result = CommunicationTxPack(pComPort->pCom, pComPort->port, 0, PACKTP_DAT, isNeedAck, wTMs, retryCnt, packId, pDat, dLen);
    return result.comRet;
}

int32_t ComPortGetPack(COM_PORT_ATTR_ST *pComPort, uint16_t packId, uint32_t wTMs, uint8_t retryCnt, void *pTxDat, uint16_t tLen, void *pBuf, uint16_t bLen)
{
    COM_RESULT_ST result = CommunicationGetPack(pComPort->pCom, pComPort->port, PACKTP_DAT, wTMs, retryCnt, packId, pTxDat, tLen, pBuf, bLen);
    return result.comRet;
}


//note: pingDlen must be mutiple of 4
COM_RESULT_ST CommunicationPing(COM_ATTR_ST *pCom, uint16_t pingDlen, uint64_t *pDtNs) // ping test
{
    uint64_t t1 = 0;
    t1 = GetMonotonicTickNs();
    uint8_t txBuf[MAX_PACK_DAT_SZ] = {0};
    uint8_t rxBuf[MAX_PACK_DAT_SZ] = {0};
    COM_RESULT_ST result = {.comRet = COM_RET_NONE};

    for(uint16_t i = 0; i < pingDlen; i++)
    {
        txBuf[i] = (uint8_t)i;
    }

    result = CommunicationGetPack(pCom, 0, PACKTP_PING_REQ, 1000, 0, 0, txBuf, pingDlen, rxBuf, pingDlen);

    *pDtNs = GetMonotonicTickNs() - t1;

    if(COM_RET_SUCCESS == result.comRet)
    {
        if(0 != memcmp(txBuf, rxBuf, pingDlen))
        {
            result.comRet = COM_RET_WAIT_FAILED;
        }
    }

    return result;
}


//note: packLen must be mutiple of 4
int32_t CommunicationTxThroughput(COM_ATTR_ST *pCom, uint32_t txSz, uint16_t packLen)// test send throughput 
{
    uint8_t dataBuf[MAX_PACK_DAT_SZ] = {0};
    int32_t sendCnt = txSz / packLen, txBdw = 0;
    uint64_t txBytes1 = 0, txBytes2 = 0;
    
    if(0 >= sendCnt)
    {
        sendCnt = 1;
    }
    
    memset(dataBuf, 'T', packLen);
    
    txBytes1 = QueueGetTotalPoped(pCom->pTransfer->pTxQueue);
    uint64_t t1 = GetMonotonicTickNs();

    for(int32_t i = 0; i < sendCnt; )
    {
        if(CalcQueueFreeSz(pCom->pTransfer->pTxQueue) >= (int32_t)(PACK_HEAD_SZ + packLen))
        {
            COM_RESULT_ST result = PackTransaction(pCom, OPTION_SETMETHOD, 0, 0, PACKTP_THP_DAT, false, 0, 0, COM_GET_TICK_US(pCom), 0, dataBuf, packLen, NULL, 0);
            if(COM_RET_SUCCESS == result.comRet)
            {
                i++;
            }
        }
        else
        {
            ThreadUsleep(100);
        }
    }
    
    txBytes2 = QueueGetTotalPoped(pCom->pTransfer->pTxQueue);
    uint64_t dt = GetMonotonicTickNs() - t1;

    txBdw = (txBytes2 - txBytes1) * (1000000000.0 / dt); // Bytes / sec
    
    return txBdw;
}


//note: packLen must be mutiple of 4
int32_t CommunicationTimedTxThp(COM_ATTR_ST *pCom, uint64_t durationMs, uint16_t packLen)// test send throughput 
{
    uint8_t dataBuf[MAX_PACK_DAT_SZ] = {0};
    int32_t txBdw = 0;
    uint64_t txBytes1 = 0, txBytes2 = 0;
    uint64_t t1 = 0;

    memset(dataBuf, 'T', packLen);
    
    txBytes1 = QueueGetTotalPoped(pCom->pTransfer->pTxQueue);

    for(t1 = GetMonotonicTickNs(); (GetMonotonicTickNs() - t1) < (durationMs * 1000000); )
    {
        if(CalcQueueFreeSz(pCom->pTransfer->pTxQueue) >= (int32_t)(PACK_HEAD_SZ + packLen))
        {
            PackTransaction(pCom, OPTION_SETMETHOD, 0, 0, PACKTP_THP_DAT, false, 0, 0, COM_GET_TICK_US(pCom), 0, dataBuf, packLen, NULL, 0);
        }
        else
        {
            ThreadUsleep(100);
        }
    }
    txBytes2 = QueueGetTotalPoped(pCom->pTransfer->pTxQueue);
    uint64_t dt = GetMonotonicTickNs() - t1;

    txBdw = (txBytes2 - txBytes1) * (1000000000.0 / dt); // Bytes / sec
    
    return txBdw;
}


//note: packLen must be mutiple of 4
int32_t CommunicationRxThroughput(COM_ATTR_ST *pCom, uint32_t rxSz, uint16_t packLen)// test receive throughput 
{
    if(packLen > MAX_PACK_DAT_SZ)
    {
        LOGE("COM RxThroughtput test failed,data len is large than MAX_PACK_DAT_SZ\r\n");
        return -1;
    }
    
    
    COM_THP_REQATTR_ST thpReq = {0};
    uint64_t rxBytes1 = 0, rxBytes2 = 0, rxBytes = 0;
    int32_t rxBdw = 0;
    uint64_t t1 = 0, t2 = 0, dt = 0;
    COM_RESULT_ST result = {.comRet = COM_RET_NONE};
    
    thpReq.packSz = H2COM16(packLen);
    thpReq.reqDatSz = H2COM32(rxSz);

    result = CommunicationTxPack(pCom, 0, 0, PACKTP_THP_REQ, true, 300, 2, 0, &thpReq, sizeof(thpReq));
    if(COM_RET_SUCCESS == result.comRet)
    {
        rxBytes1 = QueueGetTotalPushed(pCom->pTransfer->pRxQueue);
        t1 = GetMonotonicTickNs();
        
        for(; rxBytes < rxSz && dt < 5000000000;)
        {
            ThreadUsleep(1 * 1000);
            rxBytes2 = QueueGetTotalPushed(pCom->pTransfer->pRxQueue);
            t2 = GetMonotonicTickNs();
            dt = t2 - t1;
            rxBytes = rxBytes2 - rxBytes1;
        }
        
        rxBdw = rxBytes * (1000000000.0 / dt); // Bytes / sec
        return rxBdw;
    }
    return -1;
}



