#include <ldrp.h>

void
NTAPI
LdrpPinNodeRecurse(IN PLDR_DDAG_NODE Node)
{
    LDRP_ASSERT_DDAG_NODE(Node);

    const PLDR_DATA_TABLE_ENTRY Module = LDRP_FIRST_MODULE(Node);

    LDRP_ASSERT_MODULE_ENTRY(Module);
    LDRP_ASSERT_NODE_ENTRY(Node, Module);

    // fastpath
    if (LDRP_NODE_MODULE_PINNED(Node, Module))
        return;

    Node->LoadCount = LDR_LOADCOUNT_MAX;
    Module->ObsoleteLoadCount = -1;

    const PSINGLE_LIST_ENTRY ListHead = Node->Dependencies.Tail;
    for (PSINGLE_LIST_ENTRY ListEntry = ListHead ? ListHead->Next : NULL;
         ListEntry;
         ListEntry = (ListEntry != ListHead) ? ListEntry->Next : NULL)
    {
        const PLDRP_CSLIST_ENTRY DependencyEntry = CONTAINING_RECORD(ListEntry, LDRP_CSLIST_ENTRY, DependenciesLink);

        LDRP_ASSERT_CSLIST_ENTRY(DependencyEntry);

        LdrpPinNodeRecurse(DependencyEntry->DependencyNode);
    }
}

NTSTATUS
NTAPI
LdrpPinModule(IN PLDR_DATA_TABLE_ENTRY Module)
{
    LDRP_ASSERT_MODULE_ENTRY(Module);

    // If this is already a pinned module, fastpath
    if (LDRP_MODULE_PINNED(Module))
        return STATUS_SUCCESS;

    NTSTATUS Status = STATUS_SUCCESS;

    // Erase this module from double-linked list of all modules
    RtlAcquireSRWLockExclusive(&LdrpModuleDatatableLock);

    if (Module->DdagNode->LoadCount)
        LdrpPinNodeRecurse(Module->DdagNode);
    else
        Status = STATUS_UNSUCCESSFUL;

    RtlReleaseSRWLockExclusive(&LdrpModuleDatatableLock);

    return Status;
}
