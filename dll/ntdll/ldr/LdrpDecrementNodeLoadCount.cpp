#include <ldrp.h>

// Requires LdrpModuleDatatableLock
NTSTATUS
NTAPI
LdrpDecrementNodeLoadCountLockHeld(IN PLDR_DDAG_NODE Node, IN BOOLEAN DisallowOrphaning, OUT BOOLEAN* BecameOrphan)
{
    LDRP_ASSERT_DDAG_NODE(Node);

    const PLDR_DATA_TABLE_ENTRY Module = LDRP_FIRST_MODULE(Node);

    LDRP_ASSERT_MODULE_ENTRY(Module);
    LDRP_ASSERT_NODE_ENTRY(Node, Module);

    /* If this is not a pinned module */
    if (LDRP_NODE_MODULE_PINNED(Node, Module))
        return STATUS_SUCCESS;

    const ULONG32 OldLoadCount = Node->LoadCount;
    if (OldLoadCount <= (Node->IncomingDependencies.Tail != NULL ? 1u : 0u))
    {
        if (!OldLoadCount && NtCurrentTeb()->LoadOwner && Node->LoadWhileUnloadingCount)
        {
            --Node->LoadWhileUnloadingCount;
        }

        return STATUS_SUCCESS;
    }

    if (OldLoadCount <= 1 && DisallowOrphaning)
        return STATUS_RETRY;

    Node->LoadCount = OldLoadCount - 1;

    if (BecameOrphan)
        *BecameOrphan = OldLoadCount == 1;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
LdrpDecrementModuleLoadCountEx(IN PLDR_DATA_TABLE_ENTRY Module, IN BOOLEAN DisallowOrphaning)
{
    LDRP_ASSERT_MODULE_ENTRY(Module);

    BOOLEAN BecameOrphan = FALSE;

    // Fastpath
    if (DisallowOrphaning && Module->DdagNode->LoadCount == 1)
        return STATUS_RETRY;

    RtlAcquireSRWLockExclusive(&LdrpModuleDatatableLock);
    
    NTSTATUS Status = LdrpDecrementNodeLoadCountLockHeld(Module->DdagNode, DisallowOrphaning, &BecameOrphan);
    
    RtlReleaseSRWLockExclusive(&LdrpModuleDatatableLock);

    if (BecameOrphan)
    {
        RtlEnterCriticalSection(&LdrpLoaderLock);
        
        LdrpUnloadNode(Module->DdagNode);
        
        RtlLeaveCriticalSection(&LdrpLoaderLock);
    }

    return Status;
}
