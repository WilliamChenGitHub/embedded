
#pragma once

#include "EmbeddedDef.h"

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
    int8_t * name;
    JSON_VAR_TP_ET type;
    int8_t data[0];
}JSON_OBJ_ST;

typedef struct __JSON
{
    JSON_OBJ_ST *pRoot;
    int8_t * prtBuf;
    int32_t prtBufSz;
    bool isPrtStrFormat;
}JSON_ST;
//#pragma pack(pop)



#ifdef __cplusplus
extern "C"{
#endif

// create json
JSON_ST *JSONCreate(void);
bool JSONDestory(JSON_ST *pJson);

JSON_OBJ_ST *JSONCreateObj(JSON_VAR_TP_ET tp, int32_t sz, void *data, int8_t *name);
void JSONAddObj(JSON_OBJ_ST *dst, JSON_OBJ_ST *src);
bool JSONDeletObj(JSON_OBJ_ST *obj);

JSON_OBJ_ST *JSONSeek(JSON_OBJ_ST *obj, int8_t * objName);

int32_t JSONPrintf(JSON_ST *pJson, bool isEnFormat);
int32_t JSONPrint2File(JSON_ST *pJson, int8_t * fileName);

JSON_ST *JSONParseStr(int8_t * txt);
JSON_ST *JSONParseFile(int8_t * file);

int32_t JSONGetArrSz(JSON_OBJ_ST *pArr);
JSON_OBJ_ST *JSONGetArrItem(JSON_OBJ_ST *pArr, int32_t idx);

bool JSONGetObjValue(JSON_OBJ_ST *pObj, void*pBuf);

#ifdef __cplusplus
}
#endif


