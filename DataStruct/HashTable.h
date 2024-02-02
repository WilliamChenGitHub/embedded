#ifndef __HASHTABLE_H__
#define __HASHTABLE_H__

#include "stdint.h"
#include "List.h"
#include "MmMngr.h"
#include <string.h>

typedef void (*HASH_NODE_DESTORY_FT)(void *pNode);

typedef struct __HashNode
{
    LIST_ST list;
    uint32_t key;

    HASH_NODE_DESTORY_FT pNodeDestory;
    uint8_t data[0];
}HASH_NODE_ST;


typedef struct
{
    uint32_t sz; // hash table capacity
    uint32_t ocpt; // occupation
    
    LIST_ST listTbl[0];
}HASH_TABLE_ST;



#ifdef __cplusplus
extern "C" 
{
#endif

HASH_TABLE_ST *HashTableCreate(uint32_t sz);

void HashTableDestory(HASH_TABLE_ST *pHTbl);

HASH_NODE_ST *HashTableLookup(HASH_TABLE_ST *pHTbl, uint32_t key);

int32_t HashTableInsert(HASH_TABLE_ST *pHTbl, HASH_NODE_ST *pNode);

int32_t HashTableDelet(HASH_TABLE_ST *pHTbl, uint32_t key);

void HashTablePrint(HASH_TABLE_ST *pHTbl);


#ifdef __cplusplus
}
#endif

#endif


