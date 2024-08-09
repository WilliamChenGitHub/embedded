#include "List.h"
#include "stdio.h"

static inline void listMerge2Left(LIST_ST *pListLeft, LIST_ST *pListRight, CMP_LIST_NODE_FT cmpFunc)
{
    LIST_ST merge = {0};
    LIST_ST *pMergeList = &merge;

    int cmp = 0;

    if(!pListLeft || !pListRight)
    {
        return ;
    }

    LIST_ST *pL1 = pListLeft->pNext;
    LIST_ST *pL2 = pListRight->pNext;
    
    while (1)
    {
        cmp = cmpFunc(pL1, pL2);
        if(0 < cmp)
        {
            pMergeList->pNext = pL1;
            if(LIST_IS_TAIL(pListLeft, pL1))
            {
                break;
            }
            pL1 = pL1->pNext;
        }
        else
        {
            pMergeList->pNext = pL2;
            if(LIST_IS_TAIL(pListRight, pL2))
            {
                break;
            }
            pL2 = pL2->pNext;
        }
        pMergeList->pNext->pPrev = pMergeList;
        pMergeList = pMergeList->pNext;
    }

    pMergeList->pNext->pPrev = pMergeList;
    pMergeList = pMergeList->pNext;

    if(0 < cmp) // merge remains node
    {
        pMergeList->pNext = pL2;
        merge.pPrev = pListRight->pPrev;
    }
    else
    {
        pMergeList->pNext = pL1;
        merge.pPrev = pListLeft->pPrev;
    }
    pMergeList->pNext->pPrev = pMergeList;

    // replace list head
    pListLeft->pNext = merge.pNext;
    pListLeft->pPrev = merge.pPrev;
    merge.pNext->pPrev = pListLeft; 
    merge.pPrev->pNext = pListLeft;
}


LIST_ST *ListSort(LIST_ST *pList, CMP_LIST_NODE_FT cmpFunc)
{
    LIST_ST *pFast = NULL, *pSlow = NULL;
    LIST_ST split = {0};

    if(LIST_IS_EMPTY(pList))
    {
        return NULL;
    }
    else if(LIST_IS_TAIL(pList, pList->pNext))
    {
        return pList;
    }

    pSlow = pList->pNext;
    pFast = pList->pNext->pNext;

     // Split a list into two pieces
    while(!LIST_IS_TAIL(pList, pFast))
    {
        pSlow = pSlow->pNext;
        pFast = pFast->pNext;
        if(LIST_IS_TAIL(pList, pFast))
        {
            break;
        }
        pFast = pFast->pNext;
    }

    ListSplit(pList, pSlow, &split);

    //pList is left , split is right
    listMerge2Left(ListSort(pList, cmpFunc), ListSort(&split, cmpFunc), cmpFunc);

    return pList;
}

