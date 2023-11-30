
#pragma once

#include "TypeDef.h"

#define TAB_CHAR "\t"
//#define TAB_CHAR "    "

//#define ENTER_CHAR "\n"
#define ENTER_CHAR "\r\n"


typedef enum 
{
    JSON_VAR_TP_INT32,
    JSON_VAR_TP_BOOL,
    JSON_VAR_TP_FLOAT32,
    JSON_VAR_TP_STRING,
    JSON_VAR_TP_ARR,
    JSON_VAR_TP_OBJ,
    JSON_VAR_TP_BUTT,
}JSON_VAR_TP_ET;

//#pragma pack(push)
//#pragma pack(1)

typedef struct __JSON_OBJ
{
    struct __JSON_OBJ *pNext, *pChild;
    S8_T * name;
    JSON_VAR_TP_ET type;
    S8_T data[0];
}JSON_OBJ_ST;

typedef struct __JSON
{
    JSON_OBJ_ST *pRoot;
    S8_T * prtBuf;
    S32_T prtBufSz;
    BOOL_T isPrtStrFormat;
}JSON_ST;
//#pragma pack(pop)



#ifdef __cplusplus
extern "C"{
#endif

// create json
JSON_ST *JSONCreate(VOID_T);
BOOL_T JSONDestory(JSON_ST *pJson);

JSON_OBJ_ST *JSONCreateObj(JSON_VAR_TP_ET tp, S32_T sz, VOID_T *data, S8_T *name);
VOID_T JSONAddObj(JSON_OBJ_ST *dst, JSON_OBJ_ST *src);
BOOL_T JSONDeletObj(JSON_OBJ_ST *obj);

JSON_OBJ_ST *JSONSeek(JSON_OBJ_ST *obj, S8_T * objName);

S32_T JSONPrintf(JSON_ST *pJson, BOOL_T isEnFormat);
S32_T JSONPrint2File(JSON_ST *pJson, S8_T * fileName);

JSON_ST *JSONParseStr(S8_T * txt);
JSON_ST *JSONParseFile(S8_T * file);

S32_T JSONGetArrSz(JSON_OBJ_ST *pArr);
JSON_OBJ_ST *JSONGetArrItem(JSON_OBJ_ST *pArr, S32_T idx);

BOOL_T JSONGetObjValue(JSON_OBJ_ST *pObj, VOID_T*pBuf);

#ifdef __cplusplus
}
#endif


