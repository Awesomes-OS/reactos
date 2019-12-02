#include <ldrp.h>

// Requires LdrpModuleDatatableLock
NTSTATUS
NTAPI
LdrpDecrementNodeLoadCountLockHeld(IN PLDR_DDAG_NODE Node, IN BOOLEAN DisallowOrphaning, OUT BOOLEAN* BecameOrphan)
{
    PLDR_DATA_TABLE_ENTRY LdrEntry = CONTAINING_RECORD(Node, LDR_DATA_TABLE_ENTRY, NodeModuleLink);

    /* If this is not a pinned module */
    if (Node->LoadCount == LDR_LOADCOUNT_MAX || LdrEntry->ProcessStaticImport)
        return STATUS_SUCCESS;

    const ULONG32 OldLoadCount = Node->LoadCount;

    if (OldLoadCount <= 1 && DisallowOrphaning)
        return STATUS_RETRY;

    Node->LoadCount = OldLoadCount - 1;

    if (BecameOrphan)
        *BecameOrphan = OldLoadCount == 1;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
LdrpDecrementNodeLoadCountEx(IN PLDR_DATA_TABLE_ENTRY LdrEntry, IN BOOLEAN DisallowOrphaning)
{
    BOOLEAN BecameOrphan = FALSE;

    // Fastpath
    if (DisallowOrphaning && LdrEntry->DdagNode->LoadCount == 1)
        return STATUS_RETRY;

    RtlAcquireSRWLockExclusive(&LdrpModuleDatatableLock);
    
    NTSTATUS Status = LdrpDecrementNodeLoadCountLockHeld(LdrEntry->DdagNode, DisallowOrphaning, &BecameOrphan);
    
    RtlReleaseSRWLockExclusive(&LdrpModuleDatatableLock);

    if (BecameOrphan)
    {
        RtlEnterCriticalSection(&LdrpLoaderLock);
        
        LdrpUnloadNode(LdrEntry->DdagNode);
        
        RtlLeaveCriticalSection(&LdrpLoaderLock);
    }

    return Status;
}