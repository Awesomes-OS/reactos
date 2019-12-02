#include <ldrp.h>

void
NTAPI
LdrpPinNodeRecurse(IN PLDR_DDAG_NODE Node)
{
    const PLDR_DATA_TABLE_ENTRY LdrEntry = CONTAINING_RECORD(Node->Modules.Flink, LDR_DATA_TABLE_ENTRY, NodeModuleLink);

    // fastpath
    if (Node->LoadCount == LDR_LOADCOUNT_MAX || LdrEntry->ProcessStaticImport)
        return;

    Node->LoadCount = LDR_LOADCOUNT_MAX;
    LdrEntry->ObsoleteLoadCount = -1;

    const PSINGLE_LIST_ENTRY ListHead = Node->Dependencies.Tail;
    for (PSINGLE_LIST_ENTRY ListEntry = ListHead ? ListHead->Next : NULL;
         ListEntry;
         ListEntry = (ListEntry != ListHead) ? ListEntry->Next : NULL)
    {
        const PLDRP_CSLIST_ENTRY DependencyEntry = CONTAINING_RECORD(ListEntry, LDRP_CSLIST_ENTRY, DependenciesLink);
        LdrpPinNodeRecurse(DependencyEntry->DependencyNode);
    }
}

NTSTATUS
NTAPI
LdrpPinModule(IN PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    // If this is already a pinned module, fastpath
    if (LdrEntry->DdagNode->LoadCount == LDR_LOADCOUNT_MAX || LdrEntry->ProcessStaticImport)
        return STATUS_SUCCESS;

    NTSTATUS Status = STATUS_SUCCESS;

    // Erase this module from double-linked list of all modules
    RtlAcquireSRWLockExclusive(&LdrpModuleDatatableLock);

    if (LdrEntry->DdagNode->LoadCount)
        LdrpPinNodeRecurse(LdrEntry->DdagNode);
    else
        Status = STATUS_UNSUCCESSFUL;

    RtlReleaseSRWLockExclusive(&LdrpModuleDatatableLock);

    return Status;
}
