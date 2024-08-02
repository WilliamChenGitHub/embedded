#include "Log.h"
#include <stdarg.h>

LOG_ATTR_ST *gPLogAttr = NULL;


static int32_t FilePutS(LOG_ATTR_ST *pLog, char *string, int32_t len)
{
    FILE_LOG_ATTR_ST *pFLog = (FILE_LOG_ATTR_ST *)pLog;
    return fwrite(string, 1, len, pFLog->pOutF);
}


static int32_t FileGetCh(LOG_ATTR_ST *pLog)
{
    FILE_LOG_ATTR_ST *pFLog = (FILE_LOG_ATTR_ST *)pLog;
    int32_t pCh;
    if(1 == fread(&pCh, 1, 1, pFLog->pInF))
    {
        return pCh;
    }
    return -1;
}

static void FileDeinit(LOG_ATTR_ST *pLog)
{
    (void) pLog;
}

static int32_t FileInit(LOG_ATTR_ST *pLog)
{
    (void) pLog;
    return 0;
}


static int32_t FileRx(LOG_ATTR_ST *pLog, uint8_t *pDat, int32_t len)
{
    (void) pLog;
    (void) pDat;
    (void) len;
    return 0;
}

int32_t FileLOGCreate(FILE_LOG_ATTR_ST *pLogAttr, FILE *pInF, FILE *pOutF)
{
    static LOG_VTBL_ST logVptbl = 
    {
        FilePutS,
        FileGetCh,
        FileRx,
        FileDeinit,
        FileInit,
    };

    pLogAttr->attr.pVptr = &logVptbl;
    pLogAttr->pInF = pInF;
    pLogAttr->pOutF = pOutF;
    
    return 0;
}

int32_t FileLOGDestroy(FILE_LOG_ATTR_ST *pLogAttr)
{
    pLogAttr->attr.pVptr = NULL;
    pLogAttr->pInF = NULL;
    pLogAttr->pOutF = NULL;
    
    return 0;
}




int32_t LOGPrintf(  LOG_LEVEL_ET lvl, char *pFormat, ...)
{
    if(!gPLogAttr)
    {
        return -1;
    }

    if(lvl < gPLogAttr->logLevel)
    {
        return -2;
    }

    char buf[256] = {0};
    va_list valist;
    va_start(valist, pFormat);
    vsnprintf(buf, sizeof buf, pFormat, valist);
    va_end(valist);
    return gPLogAttr->pVptr->pPutS(gPLogAttr, buf, (int32_t)strlen(buf));
}



void LOGSetLevel(LOG_ATTR_ST *pLogAttr, LOG_LEVEL_ET level)
{
    pLogAttr->logLevel = level;
}


int32_t LOGInit(LOG_ATTR_ST *pLogAttr)
{
    if(pLogAttr && pLogAttr->pVptr && pLogAttr->pVptr->pInit)
    {
        gPLogAttr = pLogAttr;
        return pLogAttr->pVptr->pInit(pLogAttr);
    }
    
    return -1;
}

void LOGDeinit(LOG_ATTR_ST *pLogAttr)
{
    if(pLogAttr && pLogAttr->pVptr && pLogAttr->pVptr->pDeinit)
    {
        gPLogAttr = NULL;
        pLogAttr->pVptr->pDeinit(pLogAttr);
    }
}

int32_t LOGRx(LOG_ATTR_ST *pLogAttr, uint8_t *pDat, int32_t len)
{
    if(pLogAttr && pLogAttr->pVptr && pLogAttr->pVptr->pLogRx)
    {
        return pLogAttr->pVptr->pLogRx(pLogAttr, pDat, len);
    }

    return -1;
}

int32_t LOGGetCh(LOG_ATTR_ST *pLogAttr)
{
    if(pLogAttr && pLogAttr->pVptr && pLogAttr->pVptr->pGetC)
    {
        return pLogAttr->pVptr->pGetC(pLogAttr);
    }

    return -1;
}










