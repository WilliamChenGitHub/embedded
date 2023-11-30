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


JSON_OBJ_ST *JSONCreateObj(JSON_VAR_TP_ET tp, S32_T sz, VOID_T *data, S8_T * name)
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


JSON_ST *JSONCreate(VOID_T)
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



static inline VOID_T DestoryObj(JSON_OBJ_ST *pObj)
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
    MmMngrFree(pObj); // 销毁
}

BOOL_T JSONDestory(JSON_ST *pJson)
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

VOID_T JSONAddObj(JSON_OBJ_ST *dst, JSON_OBJ_ST *src)
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


JSON_OBJ_ST *JSONSeek(JSON_OBJ_ST *obj, S8_T * objName)
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

static inline S32_T PrintString(S8_T **pBuf, S32_T *pSz, S8_T * str)
{
    if(pBuf)
    {
        S32_T len = snprintf(*pBuf, *pSz, "%s", str);
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


static inline S32_T PrintLineHead(S8_T **buf, S32_T *bufSz, S32_T childObjDepth, BOOL_T isFormat)
{
    S32_T len = 0;
    for(S32_T i =0; isFormat && i < childObjDepth; i++)
    {
        len += PrintString(buf, bufSz, TAB_CHAR);
    }
    return len;
}

static inline S32_T PrintLineTail(S8_T **buf, S32_T *bufSz, BOOL_T isFormat)
{
    S32_T len = 0;
    if(isFormat)
    {
        len += PrintString(buf, bufSz, ENTER_CHAR);
    }
    return len;
}

static inline S32_T ObjHead2Txt(JSON_OBJ_ST *pObj, S8_T **buf, S32_T *bufSz, S32_T childObjDepth, BOOL_T isFormat)
{
    S32_T len = 0;
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
                BOOL_T val = false;
                memcpy(&val, pObj->data, sizeof val);
                len += PrintString(buf, bufSz, val? "true" : "false");
            }break;

            case JSON_VAR_TP_FLOAT32:
            {
                float val = 0;
                memcpy(&val, pObj->data, sizeof val);
                S8_T valStr[32] = {0};
                snprintf(valStr, sizeof(valStr), "%f", val);
                len += PrintString(buf, bufSz, valStr);
            }break;

            case JSON_VAR_TP_INT32:
            {
                S32_T val = 0;
                memcpy(&val, pObj->data, sizeof val);
                S8_T valStr[32] = {0};
                snprintf(valStr, sizeof(valStr), "%d", val);
                len += PrintString(buf, bufSz, valStr);
            }break;

            case JSON_VAR_TP_STRING:
            {
                S8_T * val = (S8_T *)pObj->data;
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

static inline S32_T ObjTail2Txt(JSON_OBJ_ST *pObj, S8_T **buf, S32_T *bufSz, S32_T childObjDepth, BOOL_T isFormat)
{
    S32_T len = 0;
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


static inline S32_T PrintObj(JSON_OBJ_ST *pObj, S8_T **buf, S32_T *bufSz, S32_T childObjDepth, BOOL_T isFormat)
{
    S32_T len = 0;
    if(pObj)
    {
        len += ObjHead2Txt(pObj, buf, bufSz, childObjDepth, isFormat);
        len += PrintObj(pObj->pChild, buf, bufSz, childObjDepth + 1, isFormat);
        len += ObjTail2Txt(pObj, buf, bufSz, childObjDepth, isFormat);

        len += PrintObj(pObj->pNext, buf, bufSz, childObjDepth, isFormat);
    }
    return len;
}


static inline S32_T GetJsonPrintfBufSz(JSON_ST *pJson, BOOL_T isFormat)
{
    S32_T sz = 0, childObjDepth = 0;
    sz = PrintObj(pJson->pRoot, NULL, NULL, childObjDepth, isFormat);
    return sz;
}

S32_T JSONPrintf(JSON_ST *pJson, BOOL_T isEnFormat)
{
    S32_T childObjDepth = 0, sz = 0;
    S8_T *buf = NULL;

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


static inline S32_T CreateFile(S8_T * fileName, S8_T *dat, S32_T len)
{
    if(!fileName && !dat && !len)
    {
        return -1;
    }
/*
    S32_T wLen = 0, cnt = 0;

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

S32_T JSONPrint2File(JSON_ST *pJson, S8_T * fileName)
{
    S32_T ret = -1;
    JSONPrintf(pJson, true);
    ret = CreateFile(fileName, pJson->prtBuf, pJson->prtBufSz);

    return ret;
}

static inline S8_T GetChFromString(S8_T * *pStr)
{
    S8_T ch = '\0';
    if(*pStr && **pStr)
    {
        ch = **pStr;
        *pStr += 1;
    }
    return ch;
}

static inline S32_T GetStringLen(S8_T * *pTxt)
{
    S32_T strLen = 0;
    S8_T ch = 0;

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

static inline S8_T * StringParse(S8_T * *pTxt)
{
    S32_T strLen = 0;
    S8_T * strBuf = NULL;
    S8_T * str = *pTxt;
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


static inline S32_T GetObjStrLen(S8_T * *pTxt)
{
    S8_T ch = 0;
    S32_T len = 0;

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

static inline S32_T GetArrStrLen(S8_T * *pTxt)
{
    S8_T ch = 0;
    S32_T len = 0;

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

static inline S32_T GetItemStrLen(S8_T * *pTxt)
{
    S8_T ch = 0;
    S32_T len = 0;

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


static inline S8_T * GetItemStr(S8_T * *pTxt)
{
    S8_T * str = NULL;
    S8_T * txt = *pTxt;
    S32_T len = GetItemStrLen(pTxt);

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


static inline VOID_T StringDestory(S8_T * *str)
{
    if(*str)
    {
        MmMngrFree(*str);
        *str = NULL;
    }
}

static inline VOID_T SkipWhiteSpace(S8_T * *str)
{
    S8_T ch = 0;

    do
    {
        ch = GetChFromString(str);
    }
    while (ch && ch <= 32);
}


static inline long long IntParse(S8_T * str)
{
    return atol(str);
}


static inline double FloatParse(S8_T * str)
{
    return atof(str);
}

static inline VOID_T ObjParse(S8_T * *pTxt, JSON_OBJ_ST *pObj);

static inline VOID_T ArrParse(S8_T * *pTxt, JSON_OBJ_ST *pObj)
{
    S8_T * itemStr = NULL;

    itemStr = GetItemStr(pTxt);

    while(itemStr)
    {
        S8_T * str = itemStr;
        SkipWhiteSpace(&str);
        str--;

        if('\"' == str[0])
        {
            str++;
            S8_T * str1 = StringParse(&str);
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
            BOOL_T isFloatNb = false;
            for(S32_T i = 0; str[i]; i++)
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
                S32_T iNb = IntParse(str);
                JSON_OBJ_ST *pNbObj = JSONCreateObj(JSON_VAR_TP_INT32, sizeof(iNb), &iNb, NULL);
                JSONAddObj(pObj, pNbObj);
            }
        }

        StringDestory(&itemStr);
        
        itemStr = GetItemStr(pTxt);
    }
}

static inline VOID_T ObjParse(S8_T * *pTxt, JSON_OBJ_ST *pObj)
{
    S8_T * objName = NULL;
    S8_T ch = 0;
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
                S8_T * itemStr = GetItemStr(pTxt);
                S8_T * str = itemStr;
                SkipWhiteSpace(&str);
                str--;
                
                if(0 == strncmp(str, "false", 5))
                {
                    BOOL_T val = false;
                    JSON_OBJ_ST *pValObj = JSONCreateObj(JSON_VAR_TP_BOOL, sizeof(val), &val, objName);
                    JSONAddObj(pObj, pValObj);
                }
                else if(0 == strncmp(str, "true", 4))
                {
                    BOOL_T val = true;
                    JSON_OBJ_ST *pValObj = JSONCreateObj(JSON_VAR_TP_BOOL, sizeof(val), &val, objName);
                    JSONAddObj(pObj, pValObj);
                }
                else if('\"' == str[0])
                {
                    str++;
                    S8_T * str1 = StringParse(&str);
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
                    BOOL_T isFloatNb = false;
                    for(S32_T i = 0; str[i]; i++)
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
                        S32_T iNb = IntParse(str);
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

static inline S8_T BracketCheck(S8_T * *pTxt, BOOL_T *result)
{
    S8_T ch = 0, ch2 = 0;
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


BOOL_T IsJsonTxtLegal(S8_T * txt)
{
    S8_T * str = txt;
    BOOL_T result = true;
    S8_T errBuf[256] = {0};

    BracketCheck(&str, &result);
    if(false == result)
    {
        U32_T diff = str - txt;
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
        for(U32_T i = 0; i < sizeof(errBuf); i++)
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


JSON_ST *JSONParseStr(S8_T * txt)
{
    JSON_ST *pJson = NULL;
    S8_T ch = 0;
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


JSON_ST *JSONParseFile(S8_T * file)
{
    JSON_ST *pJson = NULL;
    (VOID_T) file;
/*
    FILE *fpParse = fopen(file, "r+");

    if(fpParse)
    {
        S8_T * str = NULL;
        
        fseek(fpParse, 0, SEEK_END);
        S32_T fileSz = ftell(fpParse);
        fseek(fpParse, 0, SEEK_SET);

        str = MmMngrMalloc(fileSz);

        S32_T has_been_read = 0;

        for(has_been_read = 0; has_been_read < fileSz;)
        {
            S32_T read_len = fread(&str[has_been_read], 1, fileSz - has_been_read,fpParse);

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

S32_T JSONGetArrSz(JSON_OBJ_ST *pArr)
{
    S32_T size = 0;

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

JSON_OBJ_ST *JSONGetArrItem(JSON_OBJ_ST *pArr, S32_T idx)
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

BOOL_T JSONGetObjValue(JSON_OBJ_ST *pObj, VOID_T *pBuf)
{
    BOOL_T ret = true;
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
                memcpy(pBuf, pObj->data, sizeof(BOOL_T));
            }break;

            case JSON_VAR_TP_FLOAT32 :
            {
                memcpy(pBuf, pObj->data, sizeof(float));
            }break;

            case JSON_VAR_TP_INT32 :
            {
                memcpy(pBuf, pObj->data, sizeof(S32_T));
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


