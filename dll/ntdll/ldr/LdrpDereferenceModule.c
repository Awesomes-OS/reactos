#include <ldrp.h>
#include <winbase.h>
#include <reactos/ldrp.h>

void
NTAPI
LdrpDereferenceModule(IN PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    /* If this is not a pinned module */
    if (LdrEntry->DdagNode->LoadCount == LDR_LOADCOUNT_MAX || LdrEntry->ProcessStaticImport)
        return;

    const LONG oldReferenceCount = InterlockedExchangeAdd((LONG*)& LdrEntry->ReferenceCount, -1);

    if (oldReferenceCount != 1)
        return;

    // Erase this module from double-linked list of all modules
    RtlAcquireSRWLockExclusive(&LdrpModuleDatatableLock);

    LIST_ENTRY* previous = LdrEntry->NodeModuleLink.Blink;
    LIST_ENTRY* next = LdrEntry->NodeModuleLink.Flink;
    LIST_ENTRY* current = &LdrEntry->NodeModuleLink;

    // Verify double-linked list consistency
    RtlpCheckListEntry(previous);
    RtlpCheckListEntry(current);
    RtlpCheckListEntry(next);
    ASSERT(previous->Flink == current && next->Blink == current);

    RemoveEntryList(current);

    const PLDR_DDAG_NODE Node = LdrEntry->DdagNode;
    const PLDR_DDAG_NODE FirstNode = CONTAINING_RECORD(Node->Modules.Flink, LDR_DDAG_NODE, Modules);

    RtlReleaseSRWLockExclusive(&LdrpModuleDatatableLock);

    if (LdrEntry->TlsIndex)
    {
        LdrpReleaseTlsEntry(LdrEntry, NULL);
    }

    ASSERT(NT_SUCCESS(LdrpUnmapModule(LdrEntry)));

    /* Release the activation context if it exists and wasn't already released */
    if ((LdrEntry->EntryPointActivationContext) &&
        (LdrEntry->EntryPointActivationContext != INVALID_HANDLE_VALUE))
    {
        /* Mark it as invalid */
        RtlReleaseActivationContext(LdrEntry->EntryPointActivationContext);
        LdrEntry->EntryPointActivationContext = INVALID_HANDLE_VALUE;
    }

    /* Release the full dll name string */
    if (LdrEntry->FullDllName.Buffer)
        LdrpFreeUnicodeString(&LdrEntry->FullDllName);

    /* If this is the cached entry, invalidate it */
    if (LdrpLoadedDllHandleCache == LdrEntry)
        LdrpLoadedDllHandleCache = NULL;

    /* Finally free the entry's memory */
    RtlFreeHeap(RtlGetProcessHeap(), 0, LdrEntry);

    if (Node == FirstNode)
        LdrpDestroyNode(Node);
}
