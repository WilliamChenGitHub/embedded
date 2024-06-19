#ifndef __COMMUNICATION_H__
#define __COMMUNICATION_H__

#include "EmbeddedDef.h"
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
    uint16_t packSz;
    uint32_t reqDatSz;
}COM_THP_REQATTR_ST;

typedef struct __timeSync
{
    int64_t t1;
    int64_t t2;
    int64_t t3;
    int64_t t4;
}COM_TIME_SYNC_ST;


typedef struct __comResult
{
    int32_t comRet;
    int32_t execRet;
}COM_RESULT_ST;


#pragma pack(pop)


typedef int32_t (*COM_PACK_PARSE_FT)(struct __ComPortAttr *pComPort, struct __comPack *pPack, int32_t *pExecRet);

typedef struct __ComPortAttr
{
    LIST_ST list;
    uint8_t port;
    COM_PACK_PARSE_FT pPackParse;
    struct __CommunicationAttr *pCom;
}COM_PORT_ATTR_ST;

typedef struct __ComWaitPackAttr
{
    LIST_ST list;
    
    uint32_t waitId;
    uint16_t packId;

    uint16_t bufLen;
    void *pBuf;

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
    
    int64_t timeOffset;
    uint32_t timeErrCnt;
    uint32_t packCnt;

    int32_t exit;
    int32_t stop;
    bool isMaster;
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
    uint32_t sof; // start of frame
    uint16_t sumCheck; // from len to EOF
    uint16_t len; // data len

    uint32_t packCnt; // pack count    
    uint32_t ackNb; // Acknowledgment number
    uint16_t packId; // data pack ID
    uint8_t option; // bit7: isNeedACK, bit6: method(set/get), bit3-0:pack type
    uint8_t port; // communicaiton port number
    uint64_t timestamp; // us

    uint8_t data[0];
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

#define COM_BIGLITTLESW16(A)        ((((uint16_t)(A) & 0xff00) >> 8) | \
                                    (((uint16_t)(A) & 0x00ff) << 8))

#define COM_BIGLITTLESW32(A)        ((((uint32_t)(A) & 0xff000000) >> 24) | \
                                    (((uint32_t)(A) & 0x00ff0000) >> 8) | \
                                    (((uint32_t)(A) & 0x0000ff00) << 8) | \
                                    (((uint32_t)(A) & 0x000000ff) << 24))

#define COM_BIGLITTLESW64(A)        ((((uint64_t)(A) & 0xff00000000000000) >> 56) | \
                                    (((uint64_t)(A) & 0x00ff000000000000) >> 40) | \
                                    (((uint64_t)(A) & 0x0000ff0000000000) >> 24) | \
                                    (((uint64_t)(A) & 0x000000ff00000000) >> 8) | \
                                    (((uint64_t)(A) & 0x00000000ff000000) << 8) | \
                                    (((uint64_t)(A) & 0x0000000000ff0000) << 24) | \
                                    (((uint64_t)(A) & 0x000000000000ff00) << 40) | \
                                    (((uint64_t)(A) & 0x00000000000000ff) << 56))


#ifdef MACHINE_BYTE_ORDER_LITTLE
    #ifdef COM_BYTE_ORDER_LITTLE
        #define COMBLSW(x, bits) (x)
    #else
        #ifdef COM_BYTE_ORDER_BIG
            #define COMBLSW(x, bits) COM_BIGLITTLESW##bits(x)
        #endif
    #endif
    
#else
    #ifdef COM_BYTE_ORDER_LITTLE
        #define COMBLSW(x, bits) COM_BIGLITTLESW##bits(x)
    #else
        #ifdef COM_BYTE_ORDER_BIG
            #define COMBLSW(x, bits) (x)
        #endif
    #endif
#endif


#define COM2H64(x) COMBLSW(x, 64)
#define COM2H32(x) COMBLSW(x, 32)
#define COM2H16(x) COMBLSW(x, 16)


#define H2COM64(x) COMBLSW(x, 64)
#define H2COM32(x) COMBLSW(x, 32)
#define H2COM16(x) COMBLSW(x, 16)


#ifdef __cplusplus
extern "C"{
#endif

#define GET_SYS_TICK_US()       (GetMonotonicTickNs() / 1000)
#define COM_GET_TICK_US(pCom)   (GET_SYS_TICK_US() + (pCom->timeOffset))


COM_ATTR_ST *CommunicationInit(TRANSFER_FACTORY_ST *pTransferFactory, bool isMaster, bool createTh2Parse);

void CommunicationDeinit(COM_ATTR_ST *pCom);

// port can not be zero
int32_t CommunicationRegPort(COM_ATTR_ST *pCom, COM_PORT_ATTR_ST *pComPort, COM_PACK_PARSE_FT pFnPackParse, uint8_t port);

void CommunicationUnregPort(COM_PORT_ATTR_ST *pComPort);


COM_RESULT_ST CommunicationTxPack(COM_ATTR_ST * pCom, uint8_t port, uint32_t ackNb, uint8_t packType, bool isNeedAck, uint32_t wTMs, uint8_t retryCnt, uint16_t packId, void * pDat, uint16_t len);

COM_RESULT_ST CommunicationGetPack(COM_ATTR_ST * pCom, uint8_t port, uint8_t packType, uint32_t wTMs, uint8_t retryCnt, uint16_t packId, void * pTxDat, uint16_t tLen, void * pRxBuf, uint16_t bLen);


//note: pingDlen must be mutiple of 4
COM_RESULT_ST CommunicationPing(COM_ATTR_ST *pCom, uint16_t pingDlen, uint64_t *pDtNs); // ping test

//note: packLen must be mutiple of 4
int32_t CommunicationTxThroughput(COM_ATTR_ST *pCom, uint32_t txSz, uint16_t packLen);
int32_t CommunicationTimedTxThp(COM_ATTR_ST *pCom, uint64_t durationMs, uint16_t packLen);// test send throughput 

//note: packLen must be mutiple of 4
int32_t CommunicationRxThroughput(COM_ATTR_ST *pCom, uint32_t rxSz, uint16_t packLen);

int32_t CommunicationReset(COM_ATTR_ST *pCom);

int32_t CommunicaitonStartTimeSync(COM_ATTR_ST *pCom);

void CommunicationTryParse(COM_ATTR_ST *pCom);


int32_t ComPortRetPack(COM_PORT_ATTR_ST *pComPort, uint16_t packId, uint32_t ackNb, void *pDat, uint16_t dLen);
int32_t ComPortSndPack(COM_PORT_ATTR_ST *pComPort, uint16_t packId, bool isNeedAck, uint32_t wTMs, uint8_t retryCnt, void *pDat, uint16_t dLen);
int32_t ComPortGetPack(COM_PORT_ATTR_ST *pComPort, uint16_t packId, uint32_t wTMs, uint8_t retryCnt, void *pTxDat, uint16_t tLen, void *pBuf, uint16_t bLen);

#ifdef __cplusplus
}
#endif


#endif


