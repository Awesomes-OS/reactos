#include <ldrp.h>

PLDRP_CSLIST_ENTRY
NTAPI
LdrpRecordModuleDependency(PLDR_DATA_TABLE_ENTRY LdrEntry1,
                           PLDR_DATA_TABLE_ENTRY LdrEntry2,
                           PLDRP_CSLIST_ENTRY Dependency,
                           NTSTATUS *StatusResponse)
{
    const PLDR_DDAG_NODE Node1 = LdrEntry1->DdagNode;
    const PLDR_DDAG_NODE Node2 = LdrEntry2->DdagNode;

    ASSERT(LdrEntry1);
    ASSERT(LdrEntry2);
    ASSERT(StatusResponse);

    DPRINT1("LDR: %s([%wZ:%d], [%wZ:%d], %p, ...)\n",
            __FUNCTION__,
            &LdrEntry1->BaseDllName,
            Node1->State,
            &LdrEntry2->BaseDllName,
            Node2->State,
            Dependency);

    if (LdrpDependencyExist(Node1, Node2))
    {
        if (Node2->LoadCount > 1 && Node2->LoadCount != LDR_LOADCOUNT_MAX)
            Node2->LoadCount -= 1;

        return Dependency;
    }

    if (!Dependency)
    {
        Dependency = LdrpHeapAlloc(HEAP_ZERO_MEMORY, sizeof(LDRP_CSLIST_ENTRY));

        if (!Dependency)
        {
            *StatusResponse = STATUS_NO_MEMORY;
            return NULL;
        }
    }

    Dependency->ParentNode = Node1;
    Dependency->DependencyNode = Node2;

    PSINGLE_LIST_ENTRY Link = &Dependency->DependenciesLink;
    PSINGLE_LIST_ENTRY Tail = Node1->Dependencies.Tail;

    if (Tail)
        PushEntryList(Tail, Link);
    else
        Link->Next = Link;

    Node1->Dependencies.Tail = Link;

    Link = &Dependency->IncomingDependenciesLink;
    Tail = Node2->IncomingDependencies.Tail;

    if (Tail)
        PushEntryList(Tail, Link);
    else
        Link->Next = Link;

    Node2->IncomingDependencies.Tail = Link;

    return NULL;
}