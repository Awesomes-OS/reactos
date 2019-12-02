#include <ldrp.h>

BOOLEAN
NTAPI
LdrpDependencyExist(PLDR_DDAG_NODE Node1, PLDR_DDAG_NODE Node2)
{
    if (Node1 == Node2)
        return TRUE;

    const PLDR_DATA_TABLE_ENTRY LdrEntry2 = CONTAINING_RECORD(Node2, LDR_DATA_TABLE_ENTRY, NodeModuleLink);

    if (Node2->State == LdrModulesReadyToRun
        && (Node2->LoadCount == LDR_LOADCOUNT_MAX || LdrEntry2->ProcessStaticImport))
        return TRUE;

    const PSINGLE_LIST_ENTRY ListHead = Node1->Dependencies.Tail;
    for (PSINGLE_LIST_ENTRY ListEntry = ListHead ? ListHead->Next : NULL;
         ListEntry;
         ListEntry = (ListEntry != ListHead) ? ListEntry->Next : NULL)
    {
        const PLDRP_CSLIST_ENTRY DependencyEntry = CONTAINING_RECORD(ListEntry, LDRP_CSLIST_ENTRY, DependenciesLink);
        const PLDR_DDAG_NODE DependencyNode = DependencyEntry->DependencyNode;
        if (DependencyNode == Node2)
            return TRUE;
    }

    return FALSE;
}