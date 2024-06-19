#include "Log.h"
#include <stdarg.h>

LOG_ATTR_ST *gPLogAttr = NULL;


static void FilePutS(LOG_ATTR_ST *pLog, int8_t *string, int32_t len)
{
    FILE_LOG_ATTR_ST *pFLog = (FILE_LOG_ATTR_ST *)pLog;
    fwrite(string, 1, len, pFLog->pOutF);
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

static void FileRx(LOG_ATTR_ST *pLog, uint8_t *pDat, int32_t len)
{
    (void) pLog;
    (void) pDat;
    (void) len;
}

int32_t FileLOGCreate(LOG_ATTR_ST *pLogAttr, FILE *pInF, FILE *pOutF)
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



void LOGInit(LOG_ATTR_ST *pLogAttr)
{
    gPLogAttr = pLogAttr;
}

void LOGPrintf(  LOG_LEVEL_ET lvl, int8_t *pFormat, ...)
{
    if(!gPLogAttr)
    {
        return ;
    }

    if(lvl < gPLogAttr->logLevel)
    {
        return ;
    }

    int8_t buf[256] = {0};
    va_list valist;
    va_start(valist, pFormat);
    vsnprintf(buf, sizeof buf, pFormat, valist);
    va_end(valist);
    gPLogAttr->pVptr->pPutS(gPLogAttr, buf, (int32_t)strlen(buf));
}



void LOGSetLevel(LOG_LEVEL_ET level)
{
    gPLogAttr->logLevel = level;
}

void LOGDeinit(void)
{
    gPLogAttr->pVptr->pDeinit(gPLogAttr);
    gPLogAttr = NULL;
}

void LOGArrData(int8_t *pArrName, uint8_t *pDat, int32_t len)
{

    LOGS("%s, len = %d", pArrName, len);
    for(int32_t j = 0; j < len; j++)
    {
        if(j % 8 == 0)
        {
            LOGS("\r\n");
        }

        LOGS("0x%02x,", (uint8_t)pDat[j]);

    }
    LOGS("\r\n");
}



void LOGRx(uint8_t *pDat, int32_t len)
{
    gPLogAttr->pVptr->pLogRx(gPLogAttr, pDat, len);
}

int32_t LOGGetCh(void)
{
    return gPLogAttr->pVptr->pGetC(gPLogAttr);
}










