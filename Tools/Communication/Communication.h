#ifndef __COMMUNICATION_H__
#define __COMMUNICATION_H__

#include "TypeDef.h"
#include "TransferFactory.h"
#include "Thread.h"
#include "List.h"

struct __CommunicationAttr;
struct __ComPortAttr;
struct __comPack;


#pragma pack(push)
#pragma pack(4)

typedef struct __thpAttr
{
    U16_T packSz;
    U32_T reqDatSz;
}COM_THP_REQATTR_ST;

typedef struct __timeSync
{
    S64_T t1;
    S64_T t2;
    S64_T t3;
    S64_T t4;
}COM_TIME_SYNC_ST;


typedef struct __comResult
{
    S32_T comRet;
    S32_T execRet;
}COM_RESULT_ST;


#pragma pack(pop)


typedef S32_T (*COM_PACK_PARSE_FT)(struct __ComPortAttr *pComPort, struct __comPack *pPack, S32_T *pExecRet);

typedef struct __ComPortAttr
{
    LIST_ST list;
    U8_T port;
    COM_PACK_PARSE_FT pPackParse;
    struct __CommunicationAttr *pCom;
}COM_PORT_ATTR_ST;

typedef struct __ComWaitPackAttr
{
    LIST_ST list;
    
    U32_T waitId;
    U16_T packId;

    U16_T bufLen;
    VOID_T *pBuf;

    COM_RESULT_ST *pResult;
    EVENT_T eventWait;
}COM_WAIT_PACK_ATTR_ST;


typedef struct __CommunicationAttr
{
    TRANSFER_ST *pTransfer;

    LIST_ST portList;
    
    THREAD_T thId;
    MUTEX_T portListMtx;
    MUTEX_T packCntMtx;

    THREAD_T timeSyncthId;
    EVENT_T timeSyncEvent;
    
    LIST_ST waitPackList;
    MUTEX_T waitPackListMtx;
    
    S64_T timeOffset;
    U32_T timeErrCnt;
    U32_T packCnt;

    S32_T exit;
    S32_T stop;
    BOOL_T isMaster;
}COM_ATTR_ST;


#define COM_RET_NONE                    (1)
#define COM_RET_SUCCESS                 (0)
#define COM_RET_NOT_SUPPORT_PID         (-1)
#define COM_RET_EXEC_FAILED             (-2)
#define COM_RET_PORT_NOT_FOUND          (-3)
#define COM_RET_TIME_OUT                (-4)

#define COM_RET_DATA_EXECEEDED          (-5)
#define COM_RET_STOPED                  (-6)
#define COM_RET_TX_FAILED               (-7)
#define COM_RET_ADD_WAIT_FAILED         (-8)
#define COM_RET_WAIT_FAILED             (-9)


#define COM_SOF                         (0xDEADBEEF) // start of frame
#define MAX_PACK_DAT_SZ                 (1024)
#define PACK_HEAD_SZ                    (sizeof(COM_PACK_ST))
#define MAX_PACK_SZ_TOTAL               (PACK_HEAD_SZ + MAX_PACK_DAT_SZ) // data + COM_PACK_SZ



#define OPTION_NEEDACK_BITS             (7)
#define OPTION_METHOD_BITS              (6)
#define OPTION_PACKTP_BITS              (0)


#define OPTION_IS_NEEDACK(op)           (op & (1 << OPTION_NEEDACK_BITS))
#define OPTION_IS_GETMETHOD(op)         (op & (1 << OPTION_METHOD_BITS))
#define OPTION_IS_SETMETHOD(op)         (!(op & (1 << OPTION_METHOD_BITS)))
#define OPTION_PACKTP(op)               (op & (0x0F << OPTION_PACKTP_BITS))


#define OPTION_NEEDACK                  1
#define OPTION_GETMETHOD                1
#define OPTION_SETMETHOD                0

#define OPTION_MAKE(ndack,mthd,pt)      ((ndack << OPTION_NEEDACK_BITS) | (mthd << OPTION_METHOD_BITS) | (pt << OPTION_PACKTP_BITS))

typedef enum 
{
    PACKTP_PING_REQ,
    PACKTP_PING_RES,
    PACKTP_THP_REQ, // throughput request
    PACKTP_THP_DAT,
    PACKTP_RESET,
    PACKTP_STOP_SEND,    // stop send
    PACKTP_CONTINUE_SEND, // continue send
    PACKTP_HELLO,
    PACKTP_SYNC,       // sync
    PACKTP_SYNC_ACK,    // sync ack
    PACKTP_ACK,
    PACKTP_RET_DAT,
    PACKTP_DAT,
    PACKTP_END,
}COM_PACK_ET;

#pragma pack(push)
#pragma pack(4)

typedef struct __comPack// total len must be mutiple of 4
{
    U32_T sof; // start of frame
    U16_T sumCheck; // from len to EOF
    U16_T len; // data len

    U32_T packCnt; // pack count    
    U32_T ackNb; // Acknowledgment number
    U16_T packId; // data pack ID
    U8_T option; // bit7: isNeedACK, bit6: method(set/get), bit3-0:pack type
    U8_T port; // communicaiton port number
    U64_T timestamp; // us

    U8_T data[0];
}COM_PACK_ST;

#pragma pack(pop)



#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    #define MACHINE_BYTE_ORDER_LITTLE
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    #define MACHINE_BYTE_ORDER_BIG
#else
    #error "Unknown byte order"
#endif


#define COM_BYTE_ORDER_LITTLE
//#define COM_BYTE_ORDER_BIG

                                    
#ifdef COM_BYTE_ORDER_LITTLE
#else
    #ifdef COM_BYTE_ORDER_BIG
    #else
        #error "Unknown communication byte order"
    #endif
#endif

#define COM_BIGLITTLESW16(A)        ((((U16_T)(A) & 0xff00) >> 8) | \
                                    (((U16_T)(A) & 0x00ff) << 8))

#define COM_BIGLITTLESW32(A)        ((((U32_T)(A) & 0xff000000) >> 24) | \
                                    (((U32_T)(A) & 0x00ff0000) >> 8) | \
                                    (((U32_T)(A) & 0x0000ff00) << 8) | \
                                    (((U32_T)(A) & 0x000000ff) << 24))

#define COM_BIGLITTLESW64(A)        ((((U64_T)(A) & 0xff00000000000000) >> 56) | \
                                    (((U64_T)(A) & 0x00ff000000000000) >> 40) | \
                                    (((U64_T)(A) & 0x0000ff0000000000) >> 24) | \
                                    (((U64_T)(A) & 0x000000ff00000000) >> 8) | \
                                    (((U64_T)(A) & 0x00000000ff000000) << 8) | \
                                    (((U64_T)(A) & 0x0000000000ff0000) << 24) | \
                                    (((U64_T)(A) & 0x000000000000ff00) << 40) | \
                                    (((U64_T)(A) & 0x00000000000000ff) << 56))


#ifdef MACHINE_BYTE_ORDER_LITTLE
    #ifdef COM_BYTE_ORDER_LITTLE
        #define COM2H(x, bits) (x)
    #else
        #ifdef COM_BYTE_ORDER_BIG
            #define COM2H(x, bits) COM_BIGLITTLESW#bits(x)
        #endif
    #endif
    
#else
    #ifdef COM_BYTE_ORDER_LITTLE
        #define COM2H(x, bits) COM_BIGLITTLESW#bits(x)
    #else
        #ifdef COM_BYTE_ORDER_BIG
            #define COM2H(x, bits) (x)
        #endif
    #endif
#endif


#define COM2H64(x) COM2H(x, 64)
#define COM2H32(x) COM2H(x, 32)
#define COM2H16(x) COM2H(x, 16)


#define H2COM64(x) COM2H(x, 64)
#define H2COM32(x) COM2H(x, 32)
#define H2COM16(x) COM2H(x, 16)


#ifdef __cplusplus
extern "C"{
#endif

#define GET_SYS_TICK_US()       (GetMonotonicTickNs() / 1000)
#define COM_GET_TICK_US(pCom)   (GET_SYS_TICK_US() + (pCom->timeOffset))


COM_ATTR_ST *CommunicationInit(TRANSFER_FACTORY_ST *pTransferFactory, BOOL_T isMaster, BOOL_T createTh2Parse);

VOID_T CommunicationDeinit(COM_ATTR_ST *pCom);

// port can not be zero
S32_T CommunicationRegPort(COM_ATTR_ST *pCom, COM_PORT_ATTR_ST *pComPort, COM_PACK_PARSE_FT pFnPackParse, U8_T port);

VOID_T CommunicationUnregPort(COM_PORT_ATTR_ST *pComPort);


COM_RESULT_ST CommunicationTxPack(COM_ATTR_ST * pCom, U8_T port, U32_T ackNb, U8_T packType, BOOL_T isNeedAck, U32_T wTMs, U8_T retryCnt, U16_T packId, VOID_T * pDat, U16_T len);

COM_RESULT_ST CommunicationGetPack(COM_ATTR_ST * pCom, U8_T port, U8_T packType, U32_T wTMs, U8_T retryCnt, U16_T packId, VOID_T * pTxDat, U16_T tLen, VOID_T * pRxBuf, U16_T bLen);


//note: pingDlen must be mutiple of 4
COM_RESULT_ST CommunicationPing(COM_ATTR_ST *pCom, U16_T pingDlen, U64_T *pDtNs); // ping test

//note: packLen must be mutiple of 4
S32_T CommunicationTxThroughput(COM_ATTR_ST *pCom, U32_T txSz, U16_T packLen);
S32_T CommunicationTimedTxThp(COM_ATTR_ST *pCom, U64_T durationMs, U16_T packLen);// test send throughput 

//note: packLen must be mutiple of 4
S32_T CommunicationRxThroughput(COM_ATTR_ST *pCom, U32_T rxSz, U16_T packLen);

S32_T CommunicationReset(COM_ATTR_ST *pCom);

S32_T CommunicaitonStartTimeSync(COM_ATTR_ST *pCom);

VOID_T CommunicationTryParse(COM_ATTR_ST *pCom);


S32_T ComPortRetPack(COM_PORT_ATTR_ST *pComPort, U16_T packId, U32_T ackNb, VOID_T *pDat, U16_T dLen);
S32_T ComPortSndPack(COM_PORT_ATTR_ST *pComPort, U16_T packId, BOOL_T isNeedAck, U32_T wTMs, U8_T retryCnt, VOID_T *pDat, U16_T dLen);
S32_T ComPortGetPack(COM_PORT_ATTR_ST *pComPort, U16_T packId, U32_T wTMs, U8_T retryCnt, VOID_T *pTxDat, U16_T tLen, VOID_T *pBuf, U16_T bLen);

#ifdef __cplusplus
}
#endif


#endif


