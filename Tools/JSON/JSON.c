#include "JSON.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "MmMngr.h"

/*
Function: This Module can be use to create JSON and parse JSON files
Author: William Chen
Date: 2022/10/12
*/


JSON_OBJ_ST *JSONCreateObj(JSON_VAR_TP_ET tp, int32_t sz, void *data, char * name)
{
    JSON_OBJ_ST *pObj = MmMngrMalloc(sizeof(*pObj) + sz);
    memset(pObj, 0, sizeof(*pObj) + sz);

    pObj->type = tp;
    if(name)
    {
        pObj->name = MmMngrMalloc(strlen(name) + 1);
        memset(pObj->name, 0, strlen(name) + 1);
        sprintf(pObj->name, "%s", name);
    }
    
    switch(tp)
    {
        case JSON_VAR_TP_BOOL:
        case JSON_VAR_TP_FLOAT32:
        case JSON_VAR_TP_INT32:
        case JSON_VAR_TP_STRING:
        {
            memcpy(pObj->data, data, sz);
        }break;
        
        default : break;
    }
    return pObj;
}


JSON_ST *JSONCreate(void)
{
    JSON_ST *pJson = MmMngrMalloc(sizeof(*pJson));
    memset(pJson, 0, sizeof(*pJson));

    if(pJson)
    {
        pJson->pRoot = JSONCreateObj(JSON_VAR_TP_OBJ, 0, NULL, NULL);
        if(!pJson->pRoot)
        {
            MmMngrFree(pJson);
            pJson = NULL;
        }
    }

    return pJson;
}



static inline void DestoryObj(JSON_OBJ_ST *pObj)
{
    if(!pObj)
    {
        return ;
    }
    DestoryObj(pObj->pChild); // 先销毁子obj
    DestoryObj(pObj->pNext);
    if(pObj->name)
    {
        MmMngrFree(pObj->name);
        pObj->name = NULL;
    }
    MmMngrFree(pObj); // 销�?
}

bool JSONDestory(JSON_ST *pJson)
{
    if(pJson)
    {
        DestoryObj(pJson->pRoot);
        if(pJson->prtBuf)
        {
            MmMngrFree(pJson->prtBuf);
        }
        MmMngrFree(pJson);
        return true;
    }
    return false;
}

void JSONAddObj(JSON_OBJ_ST *dst, JSON_OBJ_ST *src)
{
    if(dst && src)
    {
        if(dst->pChild)
        {
            JSON_OBJ_ST *pTail = dst->pChild;
            while (pTail->pNext)
            {
                pTail = pTail->pNext;
            }

            pTail->pNext = src;
        }
        else
        {
            dst->pChild = src;
        }
    }
}


JSON_OBJ_ST *JSONSeek(JSON_OBJ_ST *obj, char * objName)
{
    JSON_OBJ_ST *p = NULL;
    if(!obj)
    {
        return NULL;
    }
    
    for(p = obj->pChild; p; p = p->pNext)
    {
        if(p->name)
        {
            if(0 == strcmp(objName, p->name))
            {
                return p;
            }
        }
    }
    return NULL;
}

static inline int32_t PrintString(char **pBuf, int32_t *pSz, char * str)
{
    if(pBuf)
    {
        int32_t len = snprintf(*pBuf, *pSz, "%s", str);
        if(0 < len)
        {
            *pBuf += len;
            *pSz -= len;
        }
        return len;
    }
    else
    {
        return strlen(str);
    }
}


static inline int32_t PrintLineHead(char **buf, int32_t *bufSz, int32_t childObjDepth, bool isFormat)
{
    int32_t len = 0;
    for(int32_t i =0; isFormat && i < childObjDepth; i++)
    {
        len += PrintString(buf, bufSz, TAB_CHAR);
    }
    return len;
}

static inline int32_t PrintLineTail(char **buf, int32_t *bufSz, bool isFormat)
{
    int32_t len = 0;
    if(isFormat)
    {
        len += PrintString(buf, bufSz, ENTER_CHAR);
    }
    return len;
}

static inline int32_t ObjHead2Txt(JSON_OBJ_ST *pObj, char **buf, int32_t *bufSz, int32_t childObjDepth, bool isFormat)
{
    int32_t len = 0;
    if(pObj)
    {
        if(pObj->name)
        {
            len += PrintLineHead(buf, bufSz, childObjDepth, isFormat);
            len += PrintString(buf, bufSz, "\"");
            len += PrintString(buf, bufSz, pObj->name);
            len += PrintString(buf, bufSz, "\":");
        }
        else if((JSON_VAR_TP_INT32 <= pObj->type) && (JSON_VAR_TP_STRING >= pObj->type))
        {
            if(!pObj->name)
            {
                len += PrintLineHead(buf, bufSz, childObjDepth, isFormat);
            }
        }

        switch(pObj->type)
        {
            case JSON_VAR_TP_OBJ:
            {
                if(pObj->name)
                {
                    len += PrintLineTail(buf, bufSz, isFormat);
                }
                len += PrintLineHead(buf, bufSz, childObjDepth, isFormat);
                len += PrintString(buf, bufSz, "{");
                len += PrintLineTail(buf, bufSz, isFormat);
            }break;

            case JSON_VAR_TP_ARR:
            {
                if(pObj->name)
                {
                    len += PrintLineTail(buf, bufSz, isFormat);
                }
                len += PrintLineHead(buf, bufSz, childObjDepth, isFormat);
                len += PrintString(buf, bufSz, "[");
                len += PrintLineTail(buf, bufSz, isFormat);
            }break;

            case JSON_VAR_TP_BOOL:
            {
                bool val = false;
                memcpy(&val, pObj->data, sizeof val);
                len += PrintString(buf, bufSz, val? "true" : "false");
            }break;

            case JSON_VAR_TP_FLOAT32:
            {
                float val = 0;
                memcpy(&val, pObj->data, sizeof val);
                char valStr[32] = {0};
                snprintf(valStr, sizeof(valStr), "%f", val);
                len += PrintString(buf, bufSz, valStr);
            }break;

            case JSON_VAR_TP_INT32:
            {
                int32_t val = 0;
                memcpy(&val, pObj->data, sizeof val);
                char valStr[32] = {0};
                snprintf(valStr, sizeof(valStr), "%d", val);
                len += PrintString(buf, bufSz, valStr);
            }break;

            case JSON_VAR_TP_STRING:
            {
                char * val = (char *)pObj->data;
                len += PrintString(buf, bufSz, "\"");
                len += PrintString(buf, bufSz, val);
                len += PrintString(buf, bufSz, "\"");
            }break;

            default : break;
        }

        if((JSON_VAR_TP_INT32 <= pObj->type) && (JSON_VAR_TP_STRING >= pObj->type))
        {
            if(pObj->pNext)
            {
                len += PrintString(buf, bufSz, ",");
            }
            len += PrintLineTail(buf, bufSz, isFormat);
        }
    }
    return len;
}

static inline int32_t ObjTail2Txt(JSON_OBJ_ST *pObj, char **buf, int32_t *bufSz, int32_t childObjDepth, bool isFormat)
{
    int32_t len = 0;
    if(pObj)
    {
        switch(pObj->type)
        {
            case JSON_VAR_TP_OBJ:
            {
                len += PrintLineHead(buf, bufSz, childObjDepth, isFormat);
                len += PrintString(buf, bufSz, "}");
                if(pObj->pNext)
                {
                    len += PrintString(buf, bufSz, ",");
                }
                len += PrintLineTail(buf, bufSz, isFormat);
            }break;

            case JSON_VAR_TP_ARR:
            {
                len += PrintLineHead(buf, bufSz, childObjDepth, isFormat);
                len += PrintString(buf, bufSz, "]");
                if(pObj->pNext)
                {
                    len += PrintString(buf, bufSz, ",");
                }
                len += PrintLineTail(buf, bufSz, isFormat);
            }break;

            default : break;
        }
    }
    return len;
}


static inline int32_t PrintObj(JSON_OBJ_ST *pObj, char **buf, int32_t *bufSz, int32_t childObjDepth, bool isFormat)
{
    int32_t len = 0;
    if(pObj)
    {
        len += ObjHead2Txt(pObj, buf, bufSz, childObjDepth, isFormat);
        len += PrintObj(pObj->pChild, buf, bufSz, childObjDepth + 1, isFormat);
        len += ObjTail2Txt(pObj, buf, bufSz, childObjDepth, isFormat);

        len += PrintObj(pObj->pNext, buf, bufSz, childObjDepth, isFormat);
    }
    return len;
}


static inline int32_t GetJsonPrintfBufSz(JSON_ST *pJson, bool isFormat)
{
    int32_t sz = 0, childObjDepth = 0;
    sz = PrintObj(pJson->pRoot, NULL, NULL, childObjDepth, isFormat);
    return sz;
}

int32_t JSONPrintf(JSON_ST *pJson, bool isEnFormat)
{
    int32_t childObjDepth = 0, sz = 0;
    char *buf = NULL;

    do
    {
        if(!pJson)
        {
            break;
        }

        if(pJson->prtBuf)
        {
            MmMngrFree(pJson->prtBuf);
        }
        
        sz = GetJsonPrintfBufSz(pJson, isEnFormat) + 1;
        if(!sz)
        {
            break;
        }

        buf = MmMngrMalloc(sz);
        if(!buf)
        {
            break;
        }
        memset(buf, 0, sz);
        pJson->prtBuf = buf;
        pJson->prtBufSz = sz;
        pJson->isPrtStrFormat = isEnFormat;

        PrintObj(pJson->pRoot, &buf, &sz, childObjDepth, isEnFormat);
    }
    while (0);
    
    return sz;
}


static inline int32_t CreateFile(char * fileName, char *dat, int32_t len)
{
    if(!fileName && !dat && !len)
    {
        return -1;
    }
/*
    int32_t wLen = 0, cnt = 0;

    FILE *fP = fopen(fileName, "w+");

    if(fP)
    {
        do
        {
            wLen += fwrite(&dat[wLen], 1, len - wLen, fP);
            cnt++;
        }
        while ((wLen != len) && (cnt < 3));

        fclose(fP);
        if(wLen == len)
        {
            return 0;
        }
    }
*/
    return -1;
}

int32_t JSONPrint2File(JSON_ST *pJson, char * fileName)
{
    int32_t ret = -1;
    JSONPrintf(pJson, true);
    ret = CreateFile(fileName, pJson->prtBuf, pJson->prtBufSz);

    return ret;
}

static inline char GetChFromString(char * *pStr)
{
    char ch = '\0';
    if(*pStr && **pStr)
    {
        ch = **pStr;
        *pStr += 1;
    }
    return ch;
}

static inline int32_t GetStringLen(char * *pTxt)
{
    int32_t strLen = 0;
    char ch = 0;

    ch = GetChFromString(pTxt);

    while(ch)
    {
        switch(ch)
        {
            case '\"':
            {
                return strLen;
            }break;

            case '[':
            case ']':
            case '{':
            case '}':
            case ',':
            {
                return -1;
            }break;
        }
        strLen++;
        ch = GetChFromString(pTxt);
    }
    
    return -1;
}

static inline char * StringParse(char * *pTxt)
{
    int32_t strLen = 0;
    char * strBuf = NULL;
    char * str = *pTxt;
    strLen = GetStringLen(pTxt);
    if(strLen > 0)
    {
        strBuf = MmMngrMalloc(strLen + 1);
        memset(strBuf, 0, strLen + 1);
        if(strBuf)
        {
            memcpy(strBuf, str, strLen);
        }
    }
    return strBuf;
}


static inline int32_t GetObjStrLen(char * *pTxt)
{
    char ch = 0;
    int32_t len = 0;

    ch = GetChFromString(pTxt);
    while(ch)
    {
        len++;
        switch(ch)
        {
            case '{':
            {
                len += GetObjStrLen(pTxt);
            }break;
            
            case '}':
            {
                return len;
            }break;

            default : break;
        }
        ch = GetChFromString(pTxt);
    }
    return len;
}

static inline int32_t GetArrStrLen(char * *pTxt)
{
    char ch = 0;
    int32_t len = 0;

    ch = GetChFromString(pTxt);
    while(ch)
    {
        len++;
        switch(ch)
        {
            case '[':
            {
                len += GetArrStrLen(pTxt);
            }break;

            case ']':
            {
                return len;
            }break;

            default : break;
        }
        ch = GetChFromString(pTxt);
    }
    return len;
}

static inline int32_t GetItemStrLen(char * *pTxt)
{
    char ch = 0;
    int32_t len = 0;

    ch = GetChFromString(pTxt);
    while(ch)
    {
        len++;
        switch(ch)
        {
            case '{':
            {
                len += GetObjStrLen(pTxt);
                return len;
            }break;
            case '[':
            {
                len += GetArrStrLen(pTxt);
                return len;
            }break;

            case ',':
            {
                return len;
            }break;

            default : break;
        }
        ch = GetChFromString(pTxt);
    }

    return len;
}


static inline char * GetItemStr(char * *pTxt)
{
    char * str = NULL;
    char * txt = *pTxt;
    int32_t len = GetItemStrLen(pTxt);

    if(len > 0)
    {
        str = MmMngrMalloc(len + 1);
        memset(str, 0, len + 1);
        if(str)
        {
            memcpy(str, txt, len);
        }
    }
    return str;
}


static inline void StringDestory(char * *str)
{
    if(*str)
    {
        MmMngrFree(*str);
        *str = NULL;
    }
}

static inline void SkipWhiteSpace(char * *str)
{
    char ch = 0;

    do
    {
        ch = GetChFromString(str);
    }
    while (ch && ch <= 32);
}


static inline long long IntParse(char * str)
{
    return atol(str);
}


static inline double FloatParse(char * str)
{
    return atof(str);
}

static inline void ObjParse(char * *pTxt, JSON_OBJ_ST *pObj);

static inline void ArrParse(char * *pTxt, JSON_OBJ_ST *pObj)
{
    char * itemStr = NULL;

    itemStr = GetItemStr(pTxt);

    while(itemStr)
    {
        char * str = itemStr;
        SkipWhiteSpace(&str);
        str--;

        if('\"' == str[0])
        {
            str++;
            char * str1 = StringParse(&str);
            JSON_OBJ_ST *pStrObj = JSONCreateObj(JSON_VAR_TP_STRING, strlen(str1) + 1, str1, NULL);
            JSONAddObj(pObj, pStrObj);

            StringDestory(&str1);
        }
        else if('{' == str[0])
        {
            str++;
            JSON_OBJ_ST *pObjItem = JSONCreateObj(JSON_VAR_TP_OBJ, 0, NULL, NULL);
            ObjParse(&str, pObjItem);
            JSONAddObj(pObj, pObjItem);
        }
        else if('[' == str[0])
        {
            str++;
            JSON_OBJ_ST *pObjArr = JSONCreateObj(JSON_VAR_TP_ARR, 0, NULL, NULL);
            ArrParse(&str, pObjArr);
            JSONAddObj(pObj, pObjArr);
        }
        else if('-' == str[0] || ('0' <= str[0] && '9' >= str[0]))
        {
            bool isFloatNb = false;
            for(int32_t i = 0; str[i]; i++)
            {
                if('.' == str[i])
                {
                    isFloatNb = true;
                    break;
                }
            }
            if(isFloatNb)
            {
                float fNb = FloatParse(str);
                JSON_OBJ_ST *pfNbObj = JSONCreateObj(JSON_VAR_TP_FLOAT32, sizeof(fNb), &fNb, NULL);
                JSONAddObj(pObj, pfNbObj);
            }
            else
            {
                int32_t iNb = IntParse(str);
                JSON_OBJ_ST *pNbObj = JSONCreateObj(JSON_VAR_TP_INT32, sizeof(iNb), &iNb, NULL);
                JSONAddObj(pObj, pNbObj);
            }
        }

        StringDestory(&itemStr);
        
        itemStr = GetItemStr(pTxt);
    }
}

static inline void ObjParse(char * *pTxt, JSON_OBJ_ST *pObj)
{
    char * objName = NULL;
    char ch = 0;
    do
    {
        ch = GetChFromString(pTxt);
        switch(ch)
        {
            case '\"':
            {
                objName = StringParse(pTxt);
            }break;

            case ':':
            {
                char * itemStr = GetItemStr(pTxt);
                char * str = itemStr;
                SkipWhiteSpace(&str);
                str--;
                
                if(0 == strncmp(str, "false", 5))
                {
                    bool val = false;
                    JSON_OBJ_ST *pValObj = JSONCreateObj(JSON_VAR_TP_BOOL, sizeof(val), &val, objName);
                    JSONAddObj(pObj, pValObj);
                }
                else if(0 == strncmp(str, "true", 4))
                {
                    bool val = true;
                    JSON_OBJ_ST *pValObj = JSONCreateObj(JSON_VAR_TP_BOOL, sizeof(val), &val, objName);
                    JSONAddObj(pObj, pValObj);
                }
                else if('\"' == str[0])
                {
                    str++;
                    char * str1 = StringParse(&str);
                    if(str1)
                    {
                        JSON_OBJ_ST *pStrObj = JSONCreateObj(JSON_VAR_TP_STRING, strlen(str1) + 1, str1, objName);
                        JSONAddObj(pObj, pStrObj);
                        StringDestory(&str1);
                    }
                }
                else if('{' == str[0])
                {
                    str++;
                    JSON_OBJ_ST *pObjItem = JSONCreateObj(JSON_VAR_TP_OBJ, 0, NULL, objName);
                    ObjParse(&str, pObjItem);
                    JSONAddObj(pObj, pObjItem);
                }
                else if('[' == str[0])
                {
                    str++;
                    JSON_OBJ_ST *pObjArr = JSONCreateObj(JSON_VAR_TP_ARR, 0, NULL, objName);
                    ArrParse(&str, pObjArr);
                    JSONAddObj(pObj, pObjArr);
                }
                else if('-' == str[0] || ('0' <= str[0] && '9' >= str[0]))
                {
                    bool isFloatNb = false;
                    for(int32_t i = 0; str[i]; i++)
                    {
                        if('.' == str[i])
                        {
                            isFloatNb = true;
                            break;
                        }
                    }
                    if(isFloatNb)
                    {
                        float fNb = FloatParse(str);
                        JSON_OBJ_ST *pfNbObj = JSONCreateObj(JSON_VAR_TP_FLOAT32, sizeof(fNb), &fNb, objName);
                        JSONAddObj(pObj, pfNbObj);
                    }
                    else
                    {
                        int32_t iNb = IntParse(str);
                        JSON_OBJ_ST *pNbObj = JSONCreateObj(JSON_VAR_TP_INT32, sizeof(iNb), &iNb, objName);
                        JSONAddObj(pObj, pNbObj);
                    }
                }
                
                StringDestory(&objName);
                StringDestory(&itemStr);
            }break;
        }
    }
    while(ch);
}

static inline char BracketCheck(char * *pTxt, bool *result)
{
    char ch = 0, ch2 = 0;
    do
    {
        ch = GetChFromString(pTxt);
        switch(ch)
        {
            case '{':
            {
                ch2 = BracketCheck(pTxt, result);
                if('}' != ch2)
                {
                    *result = false;
                }
            }break;
            case '[':
            {
                ch2 = BracketCheck(pTxt, result);
                if(']' != ch2)
                {
                    *result = false;
                }
            }break;

            case '}':
            case ']':
            {
                return ch;
            }break;

            case '\"':
            {
                if(0 > GetStringLen(pTxt))
                {
                    *result = false;
                }
            }break;

            default : break;
        }

    }
    while ('\0' != ch && *result);

    return ch;
}


bool IsJsonTxtLegal(char * txt)
{
    char * str = txt;
    bool result = true;
    char errBuf[256] = {0};

    BracketCheck(&str, &result);
    if(false == result)
    {
        uint32_t diff = str - txt;
        if(diff < (sizeof(errBuf) / 2))
        {
            snprintf(errBuf, sizeof(errBuf) - 1, "%s", (str - diff));
        }
        else
        {
            snprintf(errBuf, sizeof(errBuf) - 1, "%s", (str - sizeof(errBuf) / 2));
            diff = sizeof(errBuf) / 2;
        }

        //printf("\033[0;32;31m""error : \n""\033[m");
        for(uint32_t i = 0; i < sizeof(errBuf); i++)
        {
            if(diff == i + 1)
            {
                //printf("\033[0;32;31m""%c""\033[m", errBuf[i]);
            }
            else
            {
                //printf("%c", errBuf[i]);
            }
        }
        
        //printf("\n");
    }
    return result;
}


JSON_ST *JSONParseStr(char * txt)
{
    JSON_ST *pJson = NULL;
    char ch = 0;
    do
    {
        if(!txt)
        {
            break;
        }

        if(false == IsJsonTxtLegal(txt))
        {
            //printf("Json txt is illeagal\r\n");
            break;
        }

        pJson = JSONCreate();
        if(!pJson)
        {
            break;
        }

        do
        {
            ch = GetChFromString(&txt);
            if('{' == ch)
            {
                ObjParse(&txt, pJson->pRoot);
                break;
            }
        }
        while (ch);
    }while(0);
    return pJson;
}


JSON_ST *JSONParseFile(char * file)
{
    JSON_ST *pJson = NULL;
    (void) file;
/*
    FILE *fpParse = fopen(file, "r+");

    if(fpParse)
    {
        char * str = NULL;
        
        fseek(fpParse, 0, SEEK_END);
        int32_t fileSz = ftell(fpParse);
        fseek(fpParse, 0, SEEK_SET);

        str = MmMngrMalloc(fileSz);

        int32_t has_been_read = 0;

        for(has_been_read = 0; has_been_read < fileSz;)
        {
            int32_t read_len = fread(&str[has_been_read], 1, fileSz - has_been_read,fpParse);

            if(read_len < 0)
            {
                break;
            }
            else if (read_len == 0)
            {
                break;
            }

            has_been_read +=read_len;
        }

        pJson = JSONParseStr(str);
        MmMngrFree(str);
        fclose(fpParse);
    }
*/
    return pJson;
}

int32_t JSONGetArrSz(JSON_OBJ_ST *pArr)
{
    int32_t size = 0;

    do
    {
        if(!pArr)
        {
            break;
        }

        JSON_OBJ_ST *pArrItem = pArr->pChild;

        while(pArrItem)
        {
            size++;
            pArrItem = pArrItem->pNext;
        }
    }
    while (0);
    return size;
}

JSON_OBJ_ST *JSONGetArrItem(JSON_OBJ_ST *pArr, int32_t idx)
{
    JSON_OBJ_ST *pArrItem = NULL;

    do
    {
        if(!pArr)
        {
            break;
        }

        pArrItem = pArr->pChild;
        while(pArrItem && idx)
        {
            idx--;
            pArrItem = pArrItem->pNext;
        }
    }
    while (0);

    return pArrItem;
}

bool JSONGetObjValue(JSON_OBJ_ST *pObj, void *pBuf)
{
    bool ret = true;
    do
    {
        if(!pObj || !pBuf)
        {
            ret = false;
            break;
        }

        switch(pObj->type)
        {
            case JSON_VAR_TP_BOOL :
            {
                memcpy(pBuf, pObj->data, sizeof(bool));
            }break;

            case JSON_VAR_TP_FLOAT32 :
            {
                memcpy(pBuf, pObj->data, sizeof(float));
            }break;

            case JSON_VAR_TP_INT32 :
            {
                memcpy(pBuf, pObj->data, sizeof(int32_t));
            }break;

            case JSON_VAR_TP_STRING :
            {
                sprintf(pBuf, "%s", pObj->data);
            }break;

            default :
            {
                ret = false; 
            }break;
        }
    }while(0);
    return ret;
}


