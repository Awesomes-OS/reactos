#include <ldrp.h>

VOID
NTAPI
LdrpLoadContextReplaceModule(IN PLDRP_LOAD_CONTEXT LoadContext, IN PLDR_DATA_TABLE_ENTRY LoadedEntry)
{
    if (ShowSnaps)
    {
        DPRINT1("LDR: %s Replacing module in context for \"%wZ\" with \"%wZ\".\n",
                __FUNCTION__,
                &LoadContext->DllName,
                &LoadedEntry->FullDllName);
    }

    RtlAcquireSRWLockExclusive(&LdrpModuleDatatableLock);

    const PLDR_DDAG_NODE OldEntryNode = LoadContext->Entry->DdagNode;
    LoadContext->Entry = LoadedEntry;
    OldEntryNode->LoadCount = 0u;

    if (LoadedEntry->DdagNode->LoadCount != LDR_LOADCOUNT_MAX)
        ++LoadedEntry->DdagNode->LoadCount;

    if (OldEntryNode->IncomingDependencies.Tail)
    {
        const PSINGLE_LIST_ENTRY NextEntry = OldEntryNode->IncomingDependencies.Tail->Next;

        if (NextEntry == OldEntryNode->IncomingDependencies.Tail)
            OldEntryNode->IncomingDependencies.Tail = NULL;
        else
            OldEntryNode->IncomingDependencies.Tail->Next = NextEntry->Next;

        LdrpDereferenceModule(LoadedEntry);

        // The link from parent to OldEntryNode
        PLDRP_CSLIST_ENTRY IncomingDependencyListEntry =
            CONTAINING_RECORD(NextEntry, LDRP_CSLIST_ENTRY, IncomingDependenciesLink);

        const PLDR_DDAG_NODE ParentNodeOfOldEntryNode = IncomingDependencyListEntry->ParentNode;

        // Do we already have a dependency link with replacement node?
        if (LdrpDependencyExist(ParentNodeOfOldEntryNode, LoadedEntry->DdagNode))
        {
            // Remove the IncomingDependencyListEntry
            LdrpRemoveDependencyEntry(&ParentNodeOfOldEntryNode->Dependencies, &IncomingDependencyListEntry->DependenciesLink);

            // Dereference LoadedEntry node (the number of incoming dependencies has dropped)
            LdrpDecrementNodeLoadCountLockHeld(LoadedEntry->DdagNode, FALSE, NULL);

            // Destroy the link we no longer need
            LdrpHeapFree(0, IncomingDependencyListEntry);
            IncomingDependencyListEntry = NULL;
        }
        else
        {
            // No already existing dependency, add one. Reuse the link with the old (replaced) node.
            IncomingDependencyListEntry->DependencyNode = LoadedEntry->DdagNode;

            const PSINGLE_LIST_ENTRY Link = &IncomingDependencyListEntry->IncomingDependenciesLink;
            const PSINGLE_LIST_ENTRY Tail = LoadedEntry->DdagNode->IncomingDependencies.Tail;

            if (Tail)
                PushEntryList(Tail, Link);
            else
                Link->Next = Link;

            LoadedEntry->DdagNode->IncomingDependencies.Tail = Link;
        }

        if (LoadedEntry->DdagNode->State >= LdrModulesMapped || !IncomingDependencyListEntry)
        {
            const PLDR_DATA_TABLE_ENTRY ParentEntryOfOldEntryNode =
                CONTAINING_RECORD(ParentNodeOfOldEntryNode->Modules.Flink, LDR_DATA_TABLE_ENTRY, NodeModuleLink);

            LdrpDecrementCountOfDependenciesPendingMap(NULL, ParentEntryOfOldEntryNode, LoadContext, LoadedEntry);
        }
    }

    RtlReleaseSRWLockExclusive(&LdrpModuleDatatableLock);
}