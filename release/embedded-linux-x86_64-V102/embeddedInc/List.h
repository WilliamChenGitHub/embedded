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
    ListInsertFront((LIST_ST *)(entry), (LIST_ST *)(entryI))

#define LIST_INSERT_BACK(entry, entryI)\
    ListInsertBack((LIST_ST *)(entry), (LIST_ST *)(entryI))


#define LIST_DELETE(entry)\
    ListDelet((LIST_ST *)(entry))

#define LIST_NEXT_ENTRY(entry)\
        ((void *)(((LIST_ST *)(entry))->pNext))
    
#define LIST_PREV_ENTRY(entry)\
        ((void *)(((LIST_ST *)(entry))->pPrev))


#define LIST_FOREACH_FROM_HEAD(pEntry, pListHead) \
    for((pEntry) = LIST_NEXT_ENTRY(pListHead);\
        ((LIST_ST *)(pEntry)) != (pListHead);\
        (pEntry) = LIST_NEXT_ENTRY(pEntry))


#define LIST_FOREACH_FROM_TAIL(pEntry, pListHead) \
    for((pEntry) = LIST_PREV_ENTRY(pListHead);\
        ((LIST_ST *)(pEntry)) != (pListHead);\
        (pEntry) = LIST_PREV_ENTRY(pEntry))


#define LIST_FOREACH_FROM_HEAD_SAFE(pEntry, pListHead, n) \
    for((pEntry) = LIST_NEXT_ENTRY(pListHead), (n) = LIST_NEXT_ENTRY(pEntry);\
        ((LIST_ST *)(pEntry)) != (pListHead);\
        (pEntry) = (n), (n) = LIST_NEXT_ENTRY(n))

#define LIST_FOREACH_FROM_TAIL_SAFE(pEntry, pListHead, p) \
    for((pEntry) = LIST_PREV_ENTRY(pListHead), (p) = LIST_PREV_ENTRY(pEntry);\
        ((LIST_ST *)(pEntry)) != (pListHead);\
        (pEntry) = (p), (p) = LIST_PREV_ENTRY(p))


#define LIST_IS_EMPTY(list)\
    (((LIST_ST *)(list))->pNext == (list))

#define LIST_IS_TAIL(pListHead, entry)\
    (pListHead == ((LIST_ST *)(entry))->pNext)

#define LIST_SWAP(entryA, entryB)\
    ListSwap((LIST_ST *)(entryA), (LIST_ST *)(entryB))

#ifdef __cplusplus
}
#endif

#endif


