#include "Log.h"
#include <stdarg.h>

LOG_ATTR_ST *gPLogAttr = NULL;


static VOID_T FilePutS(LOG_ATTR_ST *pLog, S8_T *string, S32_T len)
{
    FILE_LOG_ATTR_ST *pFLog = (FILE_LOG_ATTR_ST *)pLog;
    fwrite(string, 1, len, pFLog->pOutF);
}


static S32_T FileGetCh(LOG_ATTR_ST *pLog)
{
    FILE_LOG_ATTR_ST *pFLog = (FILE_LOG_ATTR_ST *)pLog;
    S32_T pCh;
    if(1 == fread(&pCh, 1, 1, pFLog->pInF))
    {
        return pCh;
    }
    return -1;
}

static VOID_T FileDeinit(LOG_ATTR_ST *pLog)
{
    (VOID_T) pLog;
}

static VOID_T FileRx(LOG_ATTR_ST *pLog, U8_T *pDat, S32_T len)
{
    (VOID_T) pLog;
    (VOID_T) pDat;
    (VOID_T) len;
}

S32_T FileLOGCreate(LOG_ATTR_ST *pLogAttr, FILE *pInF, FILE *pOutF)
{
    FILE_LOG_ATTR_ST *pFLog = (FILE_LOG_ATTR_ST *)pLogAttr;
    static LOG_VTBL_ST logVptbl = 
    {
        FilePutS,
        FileGetCh,
        FileRx,
        FileDeinit,
    };

    pLogAttr->pVptr = &logVptbl;
    pFLog->pInF = pInF;
    pFLog->pOutF = pOutF;
    
    return 0;
}



VOID_T LOGInit(LOG_ATTR_ST *pLogAttr)
{
    gPLogAttr = pLogAttr;
}

VOID_T LOGPrintf(  LOG_LEVEL_ET lvl, S8_T *pFormat, ...)
{
    if(!gPLogAttr)
    {
        return ;
    }

    if(lvl < gPLogAttr->logLevel)
    {
        return ;
    }

    S8_T buf[256] = {0};
    va_list valist;
    va_start(valist, pFormat);
    vsnprintf(buf, sizeof buf, pFormat, valist);
    va_end(valist);
    gPLogAttr->pVptr->pPutS(gPLogAttr, buf, (S32_T)strlen(buf));
}



VOID_T LOGSetLevel(LOG_LEVEL_ET level)
{
    gPLogAttr->logLevel = level;
}

VOID_T LOGDeinit(VOID_T)
{
    gPLogAttr->pVptr->pDeinit(gPLogAttr);
    gPLogAttr = NULL;
}

VOID_T LOGArrData(S8_T *pArrName, U8_T *pDat, S32_T len)
{

    LOGS("%s, len = %d", pArrName, len);
    for(S32_T j = 0; j < len; j++)
    {
        if(j % 8 == 0)
        {
            LOGS("\r\n");
        }

        LOGS("0x%02x,", (U8_T)pDat[j]);

    }
    LOGS("\r\n");
}



VOID_T LOGRx(U8_T *pDat, S32_T len)
{
    gPLogAttr->pVptr->pLogRx(gPLogAttr, pDat, len);
}

S32_T LOGGetCh(VOID_T)
{
    return gPLogAttr->pVptr->pGetC(gPLogAttr);
}










