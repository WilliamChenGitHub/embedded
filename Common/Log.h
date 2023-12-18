#ifndef __LOG_H__
#define __LOG_H__

#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "Miscellaneous.h"

#define LOG_RED "\033[31;1m"
#define LOG_GREEN "\033[32;1m"
#define LOG_YELLOW "\033[33;1m"
#define LOG_BLUE "\033[34;1m"
#define LOG_PURPLE "\033[35;1m"
#define LOG_WHITE "\033[37;1m"
#define LOG_CLR_ATTR "\033[0m"

#define LOG_PATH_STDIO 0

typedef enum
{
    LOG_LEVEL_DBG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WAR,
    LOG_LEVEL_ERR,
    LOG_LEVEL_CLOSE,
    LOG_LEVEL_BUTT,
}LOG_LEVEL_ET;


#ifdef __cplusplus
extern "C"{
#endif

VOID_T LOGPrintf(LOG_LEVEL_ET lvl, S8_T *pFormat, ...);

#define PRINTF(lvl, format, ...) LOGPrintf(lvl, format, ##__VA_ARGS__)

#define LOG_PROTO(lvl, type, color, format, ...)\
    PRINTF(lvl, color"%s[%s(%d)%lldms]"format"\033[0m", type, __func__, __LINE__, GetMonotonicTickNs() / 1000000, ##__VA_ARGS__)

#define LOG_SIMPLE(lvl, format, ...)\
    PRINTF(lvl, format, ##__VA_ARGS__)


#define LOGD(format,...) LOG_PROTO(LOG_LEVEL_DBG, "D:", LOG_BLUE, format, ##__VA_ARGS__)
#define LOGI(format,...) LOG_PROTO(LOG_LEVEL_INFO, "I:", LOG_GREEN, format, ##__VA_ARGS__)
#define LOGW(format,...) LOG_PROTO(LOG_LEVEL_WAR, "W:", LOG_BLUE, format, ##__VA_ARGS__)
#define LOGE(format,...) LOG_PROTO(LOG_LEVEL_ERR, "E:", LOG_RED, format, ##__VA_ARGS__) 
#define LOGS(format,...) LOG_SIMPLE(LOG_LEVEL_BUTT, format, ##__VA_ARGS__) // highest level


struct __logAttr;


typedef VOID_T (*LOG_PUTS_FT)(struct __logAttr *pLog, S8_T *string, S32_T len);
typedef S32_T (*LOG_GETC_FT)(struct __logAttr *pLog);
typedef VOID_T (*LOG_DEINIT_FT)(struct __logAttr *pLog);
typedef VOID_T (*LOG_RX_FT)(struct __logAttr *pLog, U8_T *pDat, S32_T len);


typedef struct __logVtbl
{
    LOG_PUTS_FT pPutS;
    LOG_GETC_FT pGetC;
    LOG_RX_FT pLogRx;
    LOG_DEINIT_FT pDeinit;
}LOG_VTBL_ST;

typedef struct __logAttr
{
    LOG_VTBL_ST *pVptr;
    LOG_LEVEL_ET logLevel;
}LOG_ATTR_ST;


typedef struct
{
    LOG_ATTR_ST attr;
    FILE *pInF;
    FILE *pOutF;
}FILE_LOG_ATTR_ST;


VOID_T LOGDeinit(VOID_T);

VOID_T LOGSetLevel(LOG_LEVEL_ET level);

VOID_T LOGInit(LOG_ATTR_ST *pLogAttr);

VOID_T LOGArrData(S8_T *pArrName, U8_T *pDat, S32_T len);

VOID_T LOGRx(U8_T *pDat, S32_T len);

S32_T LOGGetCh(VOID_T);

S32_T FileLOGCreate(LOG_ATTR_ST *pLogAttr, FILE *pInF, FILE *pOutF);


#ifdef __cplusplus
}
#endif


#endif


