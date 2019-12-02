#include <ldrp.h>


VOID
NTAPI
LdrpDrainWorkQueue(VOID)
{
    for (;;)
    {
        for (; ;)
        {
            RtlEnterCriticalSection(&LdrpWorkQueueLock);

            PLDRP_LOAD_CONTEXT QueuedContext = NULL;

            if (!IsListEmpty(&LdrpWorkQueue))
            {
                const PLIST_ENTRY ListEntry = RemoveHeadList(&LdrpWorkQueue);
                QueuedContext = CONTAINING_RECORD(ListEntry, LDRP_LOAD_CONTEXT, WorkQueueListEntry);
            }

            RtlLeaveCriticalSection(&LdrpWorkQueueLock);

            if (!QueuedContext)
                break;

            LdrpProcessWork(QueuedContext);
        }

        if (IsListEmpty(&LdrpRetryQueue))
            break;

        RtlEnterCriticalSection(&LdrpWorkQueueLock);

        AppendTailList(&LdrpWorkQueue, &LdrpRetryQueue);

        RtlLeaveCriticalSection(&LdrpWorkQueueLock);
    }
}