#include <ldrp.h>
#include <ndk/extypes.h>
#include <reactos/ldrp.h>

void
NTAPI
LdrpDereferenceModule(IN PLDR_DATA_TABLE_ENTRY Module)
{
    LDRP_ASSERT_MODULE_ENTRY(Module);

    /* If this is not a pinned module */
    if (LDRP_MODULE_PINNED(Module))
        return;

    ASSERT((LONG_PTR)Module->ReferenceCount > 0);

    const LONG OldReferenceCount = InterlockedExchangeAdd((LONG*)&Module->ReferenceCount, -1);

    if (OldReferenceCount != 1)
        return;

    // Erase this module from double-linked list of all modules
    RtlAcquireSRWLockExclusive(&LdrpModuleDatatableLock);

    LIST_ENTRY* Previous = Module->NodeModuleLink.Blink;
    LIST_ENTRY* Next = Module->NodeModuleLink.Flink;
    LIST_ENTRY* Current = &Module->NodeModuleLink;

    // Verify double-linked list consistency
    RtlpCheckListEntry(Previous);
    RtlpCheckListEntry(Current);
    RtlpCheckListEntry(Next);
    ASSERT(Previous->Flink == Current && Next->Blink == Current);

    RemoveEntryList(Current);

    const PLDR_DDAG_NODE Node = Module->DdagNode;

    LDRP_ASSERT_DDAG_NODE(Node);
    LDRP_ASSERT_NODE_ENTRY(Node, Module);

    const BOOLEAN EmptyNode = IsListEmpty(&Node->Modules);

    RtlReleaseSRWLockExclusive(&LdrpModuleDatatableLock);

    if (Module->TlsIndex)
    {
        ASSERT(NT_SUCCESS(LdrpReleaseTlsEntry(Module, NULL)));
    }

    ASSERT(NT_SUCCESS(LdrpUnmapModule(Module)));

    /* Release the activation context if it exists and wasn't already released */
    if (Module->EntryPointActivationContext && Module->EntryPointActivationContext != INVALID_HANDLE_VALUE)
    {
        /* Mark it as invalid */
        RtlReleaseActivationContext(Module->EntryPointActivationContext);
        Module->EntryPointActivationContext = INVALID_HANDLE_VALUE;
    }

    /* Release the full dll name string */
    if (Module->FullDllName.Buffer)
        LdrpFreeUnicodeString(&Module->FullDllName);

    /* If this is the cached entry, invalidate it */
    if (LdrpLoadedDllHandleCache == Module)
        LdrpLoadedDllHandleCache = NULL;

    /* Finally free the entry's memory */
    LdrpFreeHeap(0, Module);

    if (EmptyNode)
        LdrpDestroyNode(Node);
}
