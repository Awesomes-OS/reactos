#include <ldrp.h>

VOID
NTAPI
LdrpQueueWork(IN PLDRP_LOAD_CONTEXT LoadContext)
{
    DPRINT1("LdrpQueueWork(" LDR_HEXPTR_FMT "[%wZ:0x%08lX])\n", LoadContext, &LoadContext->DllName, *LoadContext->LoadStatus);

    if (!NT_SUCCESS(*LoadContext->LoadStatus))
        return;

    RtlEnterCriticalSection(&LdrpWorkQueueLock);

    RtlpCheckListEntry(&LdrpWorkQueue);
    InsertHeadList(&LdrpWorkQueue, &LoadContext->QueueListEntry);

    RtlLeaveCriticalSection(&LdrpWorkQueueLock);
}
