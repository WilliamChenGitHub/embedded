#include "HashTable.h"
#include "Log.h"

HASH_TABLE_ST *HashTableCreate(uint32_t sz)
{
    if(!sz)
    {
        return NULL;
    }
    
    HASH_TABLE_ST *pHashTbl = MmMngrMalloc(sizeof(*pHashTbl) + sizeof(HASH_NODE_ST) * sz);

    if(!pHashTbl)
    {
        return NULL;
    }

    pHashTbl->sz = sz;
    pHashTbl->ocpt = 0;

    for(uint32_t i = 0; i < sz; i++)
    {
        ListInit(&pHashTbl->listTbl[i]);
    }
    
    return pHashTbl;
}

void HashTableDestory(HASH_TABLE_ST *pHTbl)
{
    HASH_NODE_ST *pNext = NULL, *pEntry = NULL;
    
    for(uint32_t i = 0; i < pHTbl->sz; i++)
    {
        LIST_FOREACH_FROM_HEAD_SAFE(pEntry, &pHTbl->listTbl[i], pNext)
        {
            LIST_DELETE(pEntry);
            if(pEntry->pNodeDestory)
            {
                pEntry->pNodeDestory(pEntry);
            }
        }
    }

    pHTbl->ocpt = 0;
    MmMngrFree(pHTbl);
}


static inline uint32_t Hash(HASH_TABLE_ST *pHTbl, uint32_t key)
{
    return key % pHTbl->sz;
}


HASH_NODE_ST *HashTableLookup(HASH_TABLE_ST *pHTbl, uint32_t key)
{
    HASH_NODE_ST *pNode = NULL;

    uint32_t idx = Hash(pHTbl, key);

    LIST_FOREACH_FROM_HEAD(pNode, &pHTbl->listTbl[idx])
    {
        if(pNode->key == key)
        {
            return pNode;
        }
    }

    return NULL;
}



int32_t HashTableInsert(HASH_TABLE_ST *pHTbl, HASH_NODE_ST *pNode)
{
    uint32_t idx = 0;

    HASH_NODE_ST *pLkup = HashTableLookup(pHTbl, pNode->key);

    if(pLkup) // has already been exist
    {
        return -1;
    }

    idx = Hash(pHTbl, pNode->key); // calculater hash value

    LIST_INSERT_BACK(&pHTbl->listTbl[idx], pNode);

    pHTbl->ocpt++;
    return 0;
}

int32_t HashTableDelet(HASH_TABLE_ST *pHTbl, uint32_t key)
{
    uint32_t idx = 0;
    HASH_NODE_ST *pNext = NULL, *pEntry = NULL;
    
    idx = Hash(pHTbl, key); // calculater hash value

    LIST_FOREACH_FROM_HEAD_SAFE(pEntry, &pHTbl->listTbl[idx], pNext)
    {
        if(pEntry->key == key)
        {
            pHTbl->ocpt--;
            LIST_DELETE(pEntry);
            if(pEntry->pNodeDestory)
            {
                pEntry->pNodeDestory(pEntry);
            }
            return 0;
        }
    }
    
    return -1;
}


void HashTablePrint(HASH_TABLE_ST *pHTbl)
{
    HASH_NODE_ST *pEntry = NULL;
    
    for(uint32_t i = 0; i < pHTbl->sz; i++)
    {
        LIST_FOREACH_FROM_HEAD(pEntry, &pHTbl->listTbl[i])
        {
            LOGI("idx = %d, key = %d\r\n", i, pEntry->key);
        }
    }
}

