#include "test.h"
#include "Log.h"


typedef struct
{
    LIST_ST list;
    S32_T idx;
}LIST_TST_ST;


VOID_T ListTst(VOID_T)
{
    LIST_ST list;
    LIST_TST_ST *pNode = NULL, *pN = NULL;
    ListInit(&list);

    for(S32_T i = 0; i < 10; i++)
    {
        LIST_TST_ST *pNew = MmMngrMalloc(sizeof(LIST_TST_ST));
        pNew->idx = i;
        LIST_INSERT_BACK(&list, pNew);
    }

    LIST_FOREACH_FROM_HEAD(pNode, &list)
    {
        LOGS("list idx = %d,", pNode->idx);
    }
    LOGI("LIST_FOREACH_FROM_HEAD\r\n");

    LIST_FOREACH_FROM_TAIL(pNode, &list)
    {
        LOGS("list idx = %d,", pNode->idx);
    }
    LOGI("LIST_FOREACH_FROM_TAIL\r\n");
    

    LIST_TST_ST *p1, *p2;
    
    p1 = LIST_NEXT_ENTRY(&list);
    p2 = LIST_PREV_ENTRY(&list);
    LIST_SWAP(p1, p2);

    p1 = LIST_NEXT_ENTRY(&list);
    p2 = LIST_PREV_ENTRY(&list);
    LIST_SWAP(p1, p2);


    p1 = LIST_NEXT_ENTRY(&list);
    p2 = LIST_NEXT_ENTRY(p2);
    LIST_SWAP(p1, p2);

    p1 = LIST_NEXT_ENTRY(&list);
    p2 = LIST_NEXT_ENTRY(p2);
    LIST_SWAP(p2, p1);

    LIST_FOREACH_FROM_HEAD(pNode, &list)
    {
        LOGS("list idx = %d,", pNode->idx);
    }
    LOGI("LIST_FOREACH_FROM_HEAD\r\n");

    LIST_FOREACH_FROM_HEAD_SAFE(pNode, &list, pN)
    {
        LIST_DELETE(pNode);
        LOGS("delet %d\r\n", pNode->idx);
        MmMngrFree(pNode);
    }
    
}

int main(int argc, char **argv)
{
    (VOID_T) argc;
    (VOID_T) argv;
    FILE_LOG_ATTR_ST log;
    FileLOGCreate((LOG_ATTR_ST *)&log, stdin, stdout);
    LOGInit((LOG_ATTR_ST *)&log);
    LOGSetLevel(LOG_LEVEL_DBG);

    LOGS("LOGS\r\n");
    
    LOGD("debug log\r\n");
    LOGI("info log\r\n");
    LOGE("error log\r\n");
    LOGW("warning log\r\n");

    S32_T i = 0;
    for(i = 0; i < 10; i++)
    {
        LOGI("for1 = %d\r\n", i);
        if(i == 5)
            goto for2;
    }

    for(i = 0; i >=0; i--)
    {
        for2:
            LOGI("for2 = %d\r\n", i);
    }

    ListTst();
    return 0;
}



