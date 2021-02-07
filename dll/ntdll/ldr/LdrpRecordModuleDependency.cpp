#include <ldrp.h>

PLDRP_CSLIST_ENTRY
NTAPI
LdrpRecordModuleDependency(IN PLDR_DATA_TABLE_ENTRY LdrEntry1,
                           IN PLDR_DATA_TABLE_ENTRY LdrEntry2,
                           IN PLDRP_CSLIST_ENTRY Dependency OPTIONAL,
                           OUT NTSTATUS* StatusResponse)
{

    LDRP_ASSERT_MODULE_ENTRY(LdrEntry1);
    LDRP_ASSERT_MODULE_ENTRY(LdrEntry2);
    LDRP_ASSERT_CSLIST_ENTRY_OPTIONAL(Dependency);
    ASSERT(StatusResponse);

    const PLDR_DDAG_NODE Node1 = LdrEntry1->DdagNode;
    const PLDR_DDAG_NODE Node2 = LdrEntry2->DdagNode;

    LDRP_ASSERT_DDAG_NODE(Node1);
    LDRP_ASSERT_DDAG_NODE(Node2);

    DPRINT1("LDR: %s([%wZ:%d], [%wZ:%d], " LDR_HEXPTR_FMT ", ...)\n",
            __FUNCTION__,
            &LdrEntry1->BaseDllName,
            Node1->State,
            &LdrEntry2->BaseDllName,
            Node2->State,
            Dependency);

    if (LdrpDependencyExist(Node1, Node2))
    {
        ASSERT(Node2->LoadCount > 1);

        if (Node2->LoadCount > 1 && Node2->LoadCount != LDR_LOADCOUNT_MAX)
            Node2->LoadCount -= 1;
    }
    else
    {
        ASSERT(Node2->LoadCount > 0);

        if (!Dependency)
        {
            Dependency = new LDRP_CSLIST_ENTRY;

            if (!Dependency)
            {
                *StatusResponse = STATUS_NO_MEMORY;
                return NULL;
            }

            LDRP_MARK_CSLIST_ENTRY(Dependency);
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

        Dependency = NULL;
    }

#ifndef NDEBUG
    ULONG_PTR ChildDepCount = 0;

    const auto* const ListHead = Node2->IncomingDependencies.Tail;
    for (const auto* ListEntry = ListHead ? ListHead->Next : NULL;
        ListEntry;
        ListEntry = ListEntry != ListHead ? ListEntry->Next : NULL)
    {
        const auto* const DependencyEntry = CONTAINING_RECORD(ListEntry, LDRP_CSLIST_ENTRY,
                                                                     IncomingDependenciesLink);

        LDRP_ASSERT_CSLIST_ENTRY(DependencyEntry);

        ASSERT(DependencyEntry->DependencyNode == Node2);

        ++ChildDepCount;
    }

    ASSERT(ChildDepCount <= Node2->LoadCount);
#endif

    return Dependency;
}
