#include <ldrp.h>

BOOLEAN
NTAPI
LdrpDependencyExist(PLDR_DDAG_NODE Node1, PLDR_DDAG_NODE Node2)
{
    LDRP_ASSERT_DDAG_NODE(Node1);
    LDRP_ASSERT_DDAG_NODE(Node2);

    if (Node1 == Node2)
        return TRUE;

    const PLDR_DATA_TABLE_ENTRY LdrEntry2 = LDRP_FIRST_MODULE(Node2);

    LDRP_ASSERT_MODULE_ENTRY(LdrEntry2);
    LDRP_ASSERT_NODE_ENTRY(Node2, LdrEntry2);

    if (Node2->State == LdrModulesReadyToRun && LDRP_NODE_MODULE_PINNED(Node2, LdrEntry2))
        return TRUE;

    const PSINGLE_LIST_ENTRY ListHead = Node1->Dependencies.Tail;
    for (PSINGLE_LIST_ENTRY ListEntry = ListHead ? ListHead->Next : NULL;
         ListEntry;
         ListEntry = ListEntry != ListHead ? ListEntry->Next : NULL)
    {
        const PLDRP_CSLIST_ENTRY DependencyEntry = CONTAINING_RECORD(ListEntry, LDRP_CSLIST_ENTRY, DependenciesLink);

        LDRP_ASSERT_CSLIST_ENTRY(DependencyEntry);

        ASSERT(DependencyEntry->ParentNode == Node1);

        const PLDR_DDAG_NODE DependencyNode = DependencyEntry->DependencyNode;

        LDRP_ASSERT_DDAG_NODE(DependencyNode);

        if (DependencyNode == Node2)
            return TRUE;
    }

    return FALSE;
}
