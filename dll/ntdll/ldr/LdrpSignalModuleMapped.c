#include <ldrp.h>

void
NTAPI
LdrpDecrementCountOfDependenciesPendingMap(PLDRP_LOAD_CONTEXT ParentLoadContext,
                                           PLDR_DATA_TABLE_ENTRY ParentLdrEntry,
                                           PLDRP_LOAD_CONTEXT ChildLoadContext,
                                           PLDR_DATA_TABLE_ENTRY ChildLdrEntry)
{
    ASSERT(ParentLoadContext || ParentLdrEntry);
    ASSERT(ChildLoadContext || ChildLdrEntry);

    if (ParentLoadContext && !ParentLdrEntry)
        ParentLdrEntry = ParentLoadContext->Entry;
    else if (!ParentLoadContext && ParentLdrEntry)
        ParentLoadContext = ParentLdrEntry->LoadContext;

    if (ChildLoadContext && !ChildLdrEntry)
        ChildLdrEntry = ChildLoadContext->Entry;
    else if (!ChildLoadContext && ChildLdrEntry)
        ChildLoadContext = ChildLdrEntry->LoadContext;

    DPRINT1("LDR: %s([%wZ], [%wZ], [%wZ], [%wZ]): dependency mapped",
            __FUNCTION__,
            &ParentLoadContext->DllName,
            &ParentLdrEntry->BaseDllName,
            &ChildLoadContext->DllName,
            &ChildLdrEntry->BaseDllName);

    if (!ParentLoadContext)
    {
        DPRINT1("LDR: %s(...): no parent context!", __FUNCTION__);
        return;
    }

    if (!ParentLoadContext->CountOfDependenciesPendingMap)
    {
        DPRINT1("LDR: %s(...): dependency mapped unexpectedly!", __FUNCTION__);
        return;
    }

    --ParentLoadContext->CountOfDependenciesPendingMap;

    if (!ParentLoadContext->CountOfDependenciesPendingMap)
    {
        ParentLdrEntry->DdagNode->State = LdrModulesSnapping;
        LdrpQueueWork(ParentLoadContext);
    }
}

void
NTAPI
LdrpSignalModuleMapped(PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    const PSINGLE_LIST_ENTRY ListHead = LdrEntry->DdagNode->IncomingDependencies.Tail;
    for (PSINGLE_LIST_ENTRY ListEntry = ListHead ? ListHead->Next : NULL;
         ListEntry;
         ListEntry = (ListEntry != ListHead) ? ListEntry->Next : NULL)
    {
        const PLDRP_CSLIST_ENTRY DependencyEntry = CONTAINING_RECORD(ListEntry, LDRP_CSLIST_ENTRY,
                                                                     IncomingDependenciesLink);

        const PLDR_DDAG_NODE DependencyNode = DependencyEntry->ParentNode;

        const PLDR_DATA_TABLE_ENTRY DependencyLdrEntry = CONTAINING_RECORD(DependencyNode,
                                                                           LDR_DATA_TABLE_ENTRY,
                                                                           NodeModuleLink);

        LdrpDecrementCountOfDependenciesPendingMap(NULL, DependencyLdrEntry, NULL, LdrEntry);
    }
}
