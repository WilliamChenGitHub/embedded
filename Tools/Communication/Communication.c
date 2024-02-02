#include "Communication.h"
#include "Log.h"

static S8_T *gPackTypeName[] = 
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


static inline S8_T *GetPackTypeName(COM_PACK_ET packType)
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
static inline U32_T CalcSum(U8_T *data, U32_T length)
{
    U32_T sum = 0;
    U32_T i = 0;
    U32_T l1 = length & 0xFFFFFFE0;
    U32_T *p = NULL;

    for (i = 0; i < l1; i += 32)
    {
        p = (U32_T *)&data[i];
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

static inline U16_T CalcPackSum(COM_PACK_ST *pPackHead, U8_T *pDat, U16_T dLen)
{
  U32_T sumOfHead = CalcSum((U8_T *)&pPackHead->packCnt, PACK_HEAD_SZ - ST_OFFSET_OF(COM_PACK_ST, packCnt)) + pPackHead->len;// 4 byte alignment
  U32_T sumOfData = CalcSum(pDat, dLen);
  
  U32_T sum = sumOfHead + sumOfData;
  
  U16_T *pSum = (U16_T *)&sum;
  
  return (pSum[0] ^ pSum[1]);
}

static VOID_T ClrCommunicationBuf(COM_ATTR_ST *pCom)
{
    TransferClrBuf(pCom->pTransfer);
}

static inline COM_WAIT_PACK_ATTR_ST *GetWaitPackFromHead(COM_ATTR_ST *pCom, U32_T waitId, U16_T packId)
{
    COM_WAIT_PACK_ATTR_ST *pWaitPack = NULL, *pRet = NULL;

    MutexLock(pCom->waitPackListMtx);
    LIST_FOREACH_FROM_HEAD(pWaitPack, &pCom->waitPackList)
    {
        if(pWaitPack->waitId == waitId && pWaitPack->packId == packId)
        {
            LIST_DELETE(pWaitPack);
            pRet = pWaitPack;
            break;
        }
    }
    MutexUnlock(pCom->waitPackListMtx);
    
    return pRet;
}

static inline COM_WAIT_PACK_ATTR_ST *GetWaitPackFromTail(COM_ATTR_ST *pCom, U32_T waitId, U16_T packId)
{
    COM_WAIT_PACK_ATTR_ST *pWaitPack = NULL, *pRet = NULL;

    MutexLock(pCom->waitPackListMtx);
    LIST_FOREACH_FROM_TAIL(pWaitPack, &pCom->waitPackList)
    {
        if(pWaitPack->waitId == waitId && pWaitPack->packId == packId)
        {
            LIST_DELETE(pWaitPack);
            pRet = pWaitPack;
            break;
        }
    }
    MutexUnlock(pCom->waitPackListMtx);
    
    return pRet;

}

static inline S32_T AddWaitPack(COM_ATTR_ST *pCom, U32_T waitId, U16_T packId, VOID_T *rcvBuf, U16_T bufLen, EVENT_T event, COM_RESULT_ST *pResult)
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

        LIST_INSERT_BACK(&pCom->waitPackList, pWaitPack);
        MutexUnlock(pCom->waitPackListMtx);
        return 0;
    }
    
    return -1;
}

static inline S32_T DeletWaitPackWithId(COM_ATTR_ST *pCom, U32_T waitId, U16_T packId)
{
    COM_WAIT_PACK_ATTR_ST *pWaitPack = GetWaitPackFromTail(pCom, waitId, packId);

    if(pWaitPack)
    {
        MmMngrFree(pWaitPack);
        return 0;
    }
    return -1;
}


static inline S32_T WaitPack(COM_ATTR_ST *pCom, S32_T wTMs, EVENT_T eventWait, COM_RESULT_ST *pResult)
{
    (VOID_T) pCom;
    S32_T ret = EventTimedWait(eventWait, wTMs);

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



static inline COM_RESULT_ST PackTransaction(COM_ATTR_ST *pCom, U8_T method, U8_T port, U32_T ackNb, U8_T packType, BOOL_T isNeedAck, U32_T wTMs, U8_T retryCnt, U64_T tPUs,  U16_T packId, VOID_T *pTxBuf, U16_T tLen, VOID_T *pRxBuf, U16_T rLen)
{
    COM_RESULT_ST result = {.comRet = COM_RET_NONE};
    TRANSFER_BUF_ST bufList[3] = {0};
    S32_T comRet = COM_RET_SUCCESS;
    EVENT_T eventWait = NULL;

    COM_PACK_ST packHead = {0};
    U32_T packCnt = 0;

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
    
    for(U8_T cnt = 0; cnt <= retryCnt; cnt++)
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



static inline COM_RESULT_ST TxSyncPack(COM_ATTR_ST *pCom, U8_T packType, VOID_T *pDat, U16_T len)
{
    return PackTransaction(pCom, OPTION_SETMETHOD, 0, 0, packType, FALSE, 0, 0, GET_SYS_TICK_US(), 0, pDat, len, NULL, 0);
}

COM_RESULT_ST CommunicationSync(COM_ATTR_ST *pCom)
{
    COM_TIME_SYNC_ST timeSync = {0};
    return TxSyncPack(pCom, PACKTP_SYNC, &timeSync, sizeof(timeSync));
}

S32_T CommunicationReset(COM_ATTR_ST *pCom)
{
    S32_T cnt = 0;

    COM_RESULT_ST ret = CommunicationTxPack(pCom, 0, 0, PACKTP_HELLO, TRUE, 1000, 0, 0, NULL, 0); // Is communication well?

    if(COM_RET_SUCCESS != ret.comRet) // if failed to do reset programa
    {
        CommunicationTxPack(pCom, 0, 0, PACKTP_STOP_SEND, FALSE, 0, 0, 0, NULL, 0); // stop
        CommunicationTxPack(pCom, 0, 0, PACKTP_RESET, FALSE, 0, 0, 0, NULL, 0);// reset
        // wait device reset completed
        for(cnt = 0; (cnt < 500) && CalcQueueDataSz(pCom->pTransfer->pTxQueue); cnt++)
        {
            ThreadUsleep(1000);
        }
        ThreadUsleep(1000 * 10);

        CommunicationTxPack(pCom, 0, 0, PACKTP_STOP_SEND, TRUE, 1000, 3, 0, NULL, 0); // stop device send message
        CommunicationTxPack(pCom, 0, 0, PACKTP_RESET, FALSE, 0, 0, 0, NULL, 0);// repeat send ensure that clear device buffer is successful
        CommunicationTxPack(pCom, 0, 0, PACKTP_RESET, FALSE, 0, 0, 0, NULL, 0);// repeat send ensure that clear device buffer is successful

        // wait device reset completed
        for(cnt = 0; (cnt < 500) && CalcQueueDataSz(pCom->pTransfer->pTxQueue); cnt++)
        {
            ThreadUsleep(1000);
        }
        ThreadUsleep(1000 * 10);

        CommunicationTxPack(pCom, 0, 0, PACKTP_CONTINUE_SEND, TRUE, 1000, 3, 0, NULL, 0); // start device send message
        ret = CommunicationTxPack(pCom, 0, 0, PACKTP_HELLO, TRUE, 1000, 3, 0, NULL, 0); //Is communication well?
        return ret.comRet;
    }
    else
    {
        return ret.comRet;
    }
}


static inline COM_RESULT_ST SendAckPack(COM_ATTR_ST *pCom, U32_T ackNb, U16_T packId, VOID_T *pDat, U16_T dLen)
{
    return CommunicationTxPack(pCom, 0, ackNb, PACKTP_ACK, FALSE, 0, 0, packId, pDat, dLen);
}

static inline COM_RESULT_ST SendPingRes(COM_ATTR_ST *pCom, U32_T ackNb, U8_T *pDat, U16_T pingDlen)
{
    return CommunicationTxPack(pCom, 0, ackNb, PACKTP_PING_RES, FALSE, 0, 0, 0, pDat, pingDlen);
}


static VOID_T *ThpReqTh(VOID_T *arg)
{
    COM_THP_REQATTR_ST *pThpAttr = arg;
    COM_ATTR_ST *pCom = *((COM_ATTR_ST **)((U8_T *)arg + sizeof(COM_THP_REQATTR_ST)));
    
    CommunicationTxThroughput(pCom, pThpAttr->reqDatSz, pThpAttr->packSz);
    
    return NULL;
}

static S32_T RunThpReq(COM_ATTR_ST *pCom, COM_THP_REQATTR_ST *pThpAttr)
{
    THREAD_T thId = 0;
    static U8_T arg[sizeof(COM_THP_REQATTR_ST) + sizeof(COM_ATTR_ST *)] = {0};
    memcpy(arg, pThpAttr, sizeof(COM_THP_REQATTR_ST));
    memcpy(&arg[sizeof(COM_THP_REQATTR_ST)], &pCom, sizeof(COM_ATTR_ST *));

    thId = ThreadCreate(ThpReqTh, arg, 2 * 1024, TRUE, 10, "ComThpThread");

    if(NULL == thId)
    {
        LOGE("RunThpReq failed\r\n");
        return -1;
    }

    return 0;
}

static VOID_T *TimeSyncTh(VOID_T *arg)
{
    COM_ATTR_ST *pCom = (COM_ATTR_ST *)arg;
    
    while(!pCom->exit)
    {
        CommunicationSync(pCom);
        EventTimedWait(pCom->timeSyncEvent, -1);
    }

    return NULL;
}

S32_T CommunicaitonStartTimeSync(COM_ATTR_ST *pCom)
{
    EventGeneration(pCom->timeSyncEvent);
    return 0;
}


static VOID_T *CommunicationProcThread(VOID_T *argument)
{
    COM_ATTR_ST *pCom = (COM_ATTR_ST *)argument;
    COM_PACK_ST *pPack = NULL;
    U8_T buffer[MAX_PACK_SZ_TOTAL] = {0};
    U16_T packLen = 0;
    U16_T sumCheck = 0;
    S32_T reqLen = 0, getlen = 0;
    
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
                    S32_T stop = pCom->stop;
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
                LIST_FOREACH_FROM_HEAD(pPort, &pCom->portList)
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


COM_ATTR_ST *CommunicationInit(TRANSFER_FACTORY_ST *pTransferFactory, BOOL_T isMaster, BOOL_T createTh2Parse)
{
    COM_ATTR_ST *pCom = NULL;
    TRANSFER_ST *pTransfer = NULL;
    S32_T ret = 0;
    
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
        pCom->thId = ThreadCreate(CommunicationProcThread, pCom, 3 * 1024, FALSE, 3, "CommunicationProc");

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

        pCom->timeSyncthId = ThreadCreate(TimeSyncTh, pCom, 2 * 1024, FALSE, 10, "ComTimeSync");
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


VOID_T CommunicationTryParse(COM_ATTR_ST *pCom)
{
    COM_PACK_ST *pPack = NULL;
    U8_T buffer[MAX_PACK_SZ_TOTAL] = {0};
    U16_T packLen = 0;
    U16_T sumCheck = 0;
    S32_T reqLen = 0, getlen = 0;
    
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
                    S32_T stop = pCom->stop;
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
                LIST_FOREACH_FROM_HEAD(pPort, &pCom->portList)
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


VOID_T CommunicationDeinit(COM_ATTR_ST *pCom)
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
S32_T CommunicationRegPort(COM_ATTR_ST *pCom, COM_PORT_ATTR_ST *pComPort, COM_PACK_PARSE_FT pFnPackParse, U8_T port)
{
    if(!pCom || !pComPort)
    {
        return -1;
    }
    
    pComPort->port = port;
    pComPort->pCom = pCom;
    pComPort->pPackParse = pFnPackParse;

    MutexLock(pCom->portListMtx);
    LIST_INSERT_BACK(&pCom->portList, pComPort);
    MutexUnlock(pCom->portListMtx);

    return 0;
}

VOID_T CommunicationUnregPort(COM_PORT_ATTR_ST *pComPort)
{
    if(!pComPort || !pComPort->pCom)
    {
        return;
    }
    
    MutexLock(pComPort->pCom->portListMtx);
    LIST_DELETE(pComPort);
    MutexUnlock(pComPort->pCom->portListMtx);
}




// send a pack
// pCom: communication object, packId: pack ID, ackNb: if is ack pack , ackNb is require, ackNb equal pack count + 1. packType: reference COM_PACK_ET 
//port: port number, isNeedAck: is ack pack require, wTMs: wait time(ms), pDat: paylod, len: payload len 

COM_RESULT_ST CommunicationTxPack(COM_ATTR_ST *pCom, U8_T port, U32_T ackNb, U8_T packType, BOOL_T isNeedAck, U32_T wTMs, U8_T retryCnt, U16_T packId, VOID_T *pDat, U16_T len)
{
    return PackTransaction(pCom, OPTION_SETMETHOD, port, ackNb, packType, isNeedAck, wTMs, retryCnt, COM_GET_TICK_US(pCom), packId, pDat, len, NULL, 0);
}

// get a pack
// pCom: communication object, packId: pack ID, ackNb: if is ack pack , packType: reference COM_PACK_ET 
//port: port number,  wTMs: wait time(ms), pBuf: paylod buffer, len: payload buffer len 
COM_RESULT_ST CommunicationGetPack(COM_ATTR_ST *pCom, U8_T port, U8_T packType, U32_T wTMs, U8_T retryCnt, U16_T packId, VOID_T *pTxDat, U16_T tLen, VOID_T *pRxBuf, U16_T bLen)
{
    return PackTransaction(pCom, OPTION_GETMETHOD, port, 0, packType, TRUE, wTMs, retryCnt, COM_GET_TICK_US(pCom), packId, pTxDat, tLen, pRxBuf, bLen);
}


S32_T ComPortRetPack(COM_PORT_ATTR_ST *pComPort, U16_T packId, U32_T ackNb, VOID_T *pDat, U16_T dLen)
{
    COM_RESULT_ST result = CommunicationTxPack(pComPort->pCom, pComPort->port, ackNb, PACKTP_RET_DAT, FALSE, 0, 0, packId, pDat, dLen);
    return result.comRet;
}

S32_T ComPortSndPack(COM_PORT_ATTR_ST *pComPort, U16_T packId, BOOL_T isNeedAck, U32_T wTMs, U8_T retryCnt, VOID_T *pDat, U16_T dLen)
{
    COM_RESULT_ST result = CommunicationTxPack(pComPort->pCom, pComPort->port, 0, PACKTP_DAT, isNeedAck, wTMs, retryCnt, packId, pDat, dLen);
    return result.comRet;
}

S32_T ComPortGetPack(COM_PORT_ATTR_ST *pComPort, U16_T packId, U32_T wTMs, U8_T retryCnt, VOID_T *pTxDat, U16_T tLen, VOID_T *pBuf, U16_T bLen)
{
    COM_RESULT_ST result = CommunicationGetPack(pComPort->pCom, pComPort->port, PACKTP_DAT, wTMs, retryCnt, packId, pTxDat, tLen, pBuf, bLen);
    return result.comRet;
}


//note: pingDlen must be mutiple of 4
COM_RESULT_ST CommunicationPing(COM_ATTR_ST *pCom, U16_T pingDlen, U64_T *pDtNs) // ping test
{
    U64_T t1 = 0;
    t1 = GetMonotonicTickNs();
    U8_T txBuf[MAX_PACK_DAT_SZ] = {0};
    U8_T rxBuf[MAX_PACK_DAT_SZ] = {0};
    COM_RESULT_ST result = {.comRet = COM_RET_NONE};

    for(U16_T i = 0; i < pingDlen; i++)
    {
        txBuf[i] = (U8_T)i;
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
S32_T CommunicationTxThroughput(COM_ATTR_ST *pCom, U32_T txSz, U16_T packLen)// test send throughput 
{
    U8_T dataBuf[MAX_PACK_DAT_SZ] = {0};
    S32_T sendCnt = txSz / packLen, txBdw = 0;
    U64_T txBytes1 = 0, txBytes2 = 0;
    
    if(0 >= sendCnt)
    {
        sendCnt = 1;
    }
    
    memset(dataBuf, 'T', packLen);
    
    txBytes1 = QueueGetTotalPoped(pCom->pTransfer->pTxQueue);
    U64_T t1 = GetMonotonicTickNs();

    for(S32_T i = 0; i < sendCnt; )
    {
        if(CalcQueueFreeSz(pCom->pTransfer->pTxQueue) >= (S32_T)(PACK_HEAD_SZ + packLen))
        {
            COM_RESULT_ST result = PackTransaction(pCom, OPTION_SETMETHOD, 0, 0, PACKTP_THP_DAT, FALSE, 0, 0, COM_GET_TICK_US(pCom), 0, dataBuf, packLen, NULL, 0);
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
    U64_T dt = GetMonotonicTickNs() - t1;

    txBdw = (txBytes2 - txBytes1) * (1000000000.0 / dt); // Bytes / sec
    
    return txBdw;
}


//note: packLen must be mutiple of 4
S32_T CommunicationTimedTxThp(COM_ATTR_ST *pCom, U64_T durationMs, U16_T packLen)// test send throughput 
{
    U8_T dataBuf[MAX_PACK_DAT_SZ] = {0};
    S32_T txBdw = 0;
    U64_T txBytes1 = 0, txBytes2 = 0;
    U64_T t1 = 0;

    memset(dataBuf, 'T', packLen);
    
    txBytes1 = QueueGetTotalPoped(pCom->pTransfer->pTxQueue);

    for(t1 = GetMonotonicTickNs(); (GetMonotonicTickNs() - t1) < (durationMs * 1000000); )
    {
        if(CalcQueueFreeSz(pCom->pTransfer->pTxQueue) >= (S32_T)(PACK_HEAD_SZ + packLen))
        {
            PackTransaction(pCom, OPTION_SETMETHOD, 0, 0, PACKTP_THP_DAT, FALSE, 0, 0, COM_GET_TICK_US(pCom), 0, dataBuf, packLen, NULL, 0);
        }
        else
        {
            ThreadUsleep(100);
        }
    }
    txBytes2 = QueueGetTotalPoped(pCom->pTransfer->pTxQueue);
    U64_T dt = GetMonotonicTickNs() - t1;

    txBdw = (txBytes2 - txBytes1) * (1000000000.0 / dt); // Bytes / sec
    
    return txBdw;
}


//note: packLen must be mutiple of 4
S32_T CommunicationRxThroughput(COM_ATTR_ST *pCom, U32_T rxSz, U16_T packLen)// test receive throughput 
{
    if(packLen > MAX_PACK_DAT_SZ)
    {
        LOGE("COM RxThroughtput test failed,data len is large than MAX_PACK_DAT_SZ\r\n");
        return -1;
    }
    
    
    COM_THP_REQATTR_ST thpReq = {0};
    U64_T rxBytes1 = 0, rxBytes2 = 0, rxBytes = 0;
    S32_T rxBdw = 0;
    U64_T t1 = 0, t2 = 0, dt = 0;
    COM_RESULT_ST result = {.comRet = COM_RET_NONE};
    
    thpReq.packSz = H2COM16(packLen);
    thpReq.reqDatSz = H2COM32(rxSz);

    result = CommunicationTxPack(pCom, 0, 0, PACKTP_THP_REQ, TRUE, 300, 2, 0, &thpReq, sizeof(thpReq));
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



