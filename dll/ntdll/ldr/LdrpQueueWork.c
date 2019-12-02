#include <ldrp.h>

VOID
NTAPI
LdrpQueueWork(IN PLDRP_LOAD_CONTEXT LoadContext)
{
    if (!NT_SUCCESS(*LoadContext->StatusResponse))
        return;

    RtlEnterCriticalSection(&LdrpWorkQueueLock);

    RtlpCheckListEntry(&LdrpWorkQueue);
    InsertHeadList(&LdrpWorkQueue, &LoadContext->WorkQueueListEntry);

    RtlLeaveCriticalSection(&LdrpWorkQueueLock);
}