#ifndef __LIST_H__
#define __LIST_H__

#include "EmbeddedDef.h"

typedef struct __List
{
    struct __List *pPrev;
    struct __List *pNext;
}LIST_ST;


#ifdef __cplusplus
extern "C" 
{
#endif

static inline void ListInit(LIST_ST *pList)
{
    pList->pNext = pList->pPrev = pList;
}

static inline void ListInsertFront(LIST_ST *pEntry, LIST_ST *pEntryI)
{
    pEntryI->pPrev = pEntry->pPrev;
    pEntryI->pNext = pEntry;

    pEntry->pPrev->pNext = pEntryI;
    pEntry->pPrev = pEntryI;
}

static inline void ListInsertBack(LIST_ST *pEntry, LIST_ST *pEntryI)
{
    pEntryI->pNext = pEntry->pNext;
    pEntryI->pPrev = pEntry;

    pEntry->pNext->pPrev = pEntryI;
    pEntry->pNext = pEntryI;
}


static inline void ListDelet(LIST_ST *pEntry)
{
    pEntry->pNext->pPrev = pEntry->pPrev;
    pEntry->pPrev->pNext = pEntry->pNext;
}

static inline void ListSwap(LIST_ST *pEntryA, LIST_ST *pEntryB)
{
    if(pEntryA->pNext == pEntryB) // a right neighbour
    {
        pEntryA->pPrev->pNext = pEntryB;
        pEntryB->pNext->pPrev = pEntryA;
        
        pEntryA->pNext = pEntryB->pNext;
        pEntryB->pPrev = pEntryA->pPrev;

        pEntryA->pPrev = pEntryB;
        pEntryB->pNext = pEntryA;
    }
    else if(pEntryA->pPrev == pEntryB) // a left neighbour
    {
        pEntryB->pPrev->pNext = pEntryA;
        pEntryA->pNext->pPrev = pEntryB;

        pEntryB->pNext = pEntryA->pNext;
        pEntryA->pPrev = pEntryB->pPrev;

        pEntryB->pPrev = pEntryA;
        pEntryA->pNext = pEntryB;

    }
    else // not a neighbour
    {
        LIST_ST *p;
        pEntryA->pNext->pPrev = pEntryB;
        pEntryA->pPrev->pNext = pEntryB;

        pEntryB->pNext->pPrev = pEntryA;
        pEntryB->pPrev->pNext = pEntryA;
        
        p = pEntryA->pNext;
        pEntryA->pNext = pEntryB->pNext;
        pEntryB->pNext = p;

        p = pEntryA->pPrev;
        pEntryA->pPrev = pEntryB->pPrev;
        pEntryB->pPrev = p;
    }
}


#define LIST_INSERT_FRONT(entry, entryI)\
    ListInsertFront(entry, entryI)

#define LIST_INSERT_BACK(entry, entryI)\
    ListInsertBack(entry, entryI)


#define LIST_DELETE(entry)\
    ListDelet(entry)


#define LIST_ENTRY(ptr, type, member) \
    ST_CONTAINER_OF(ptr, type, member)


#define LIST_NEXT_ENTRY(pos, type, member)\
    LIST_ENTRY((pos)->pNext, type, member)

#define LIST_PREV_ENTRY(pos, type, member)\
    LIST_ENTRY((pos)->pPrev, type, member)



#define LIST_FOREACH_FROM_HEAD(pEntry, pListHead, member) \
    for(pEntry = LIST_NEXT_ENTRY(pListHead, typeof (*pEntry), member);\
        &(pEntry)->member != (pListHead);\
        pEntry = LIST_NEXT_ENTRY(&(pEntry)->member, typeof (*pEntry), member))

#define LIST_FOREACH_FROM_TAIL(pEntry, pListHead, member) \
    for(pEntry = LIST_PREV_ENTRY(pListHead, typeof (*pEntry), member);\
        &(pEntry)->member != (pListHead);\
        pEntry = LIST_PREV_ENTRY(&(pEntry)->member, typeof (*pEntry), member))



#define LIST_FOREACH_FROM_HEAD_SAFE(pEntry, pListHead, n, member) \
    for(pEntry = LIST_NEXT_ENTRY(pListHead, typeof (*pEntry), member),\
        n = LIST_NEXT_ENTRY(&(pEntry)->member, typeof (*pEntry), member);\
        &(pEntry)->member != (pListHead);\
        (pEntry) = (n), (n) = LIST_NEXT_ENTRY(&(n)->member, typeof (*pEntry), member))


#define LIST_FOREACH_FROM_TAIL_SAFE(pEntry, pListHead, p, member) \
    for(pEntry = LIST_PREV_ENTRY(pListHead, typeof (*pEntry), member),\
        p = LIST_PREV_ENTRY(&(pEntry)->member, typeof (*pEntry), member);\
        &(pEntry)->member != (pListHead);\
        (pEntry) = (p), (p) = LIST_PREV_ENTRY(&(p)->member, typeof (*pEntry), member))


#define LIST_IS_EMPTY(list)\
    ((list)->pNext == (list))

#define LIST_IS_TAIL(pListHead, entry)\
    (pListHead == (entry)->pNext)

#define LIST_SWAP(entryA, entryB)\
    ListSwap(entryA, entryB)

#ifdef __cplusplus
}
#endif

#endif


