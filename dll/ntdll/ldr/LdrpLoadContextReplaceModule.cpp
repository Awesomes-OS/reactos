#include <ldrp.h>

VOID
NTAPI
LdrpLoadContextReplaceModule(IN PLDRP_LOAD_CONTEXT LoadContext, IN PLDR_DATA_TABLE_ENTRY ExistingModule)
{
    LDRP_ASSERT_LOAD_CONTEXT(LoadContext);
    LDRP_ASSERT_MODULE_ENTRY(ExistingModule);

    if (LdrpDebugFlags.LogTrace)
    {
        DPRINT1("LDR: %s Replacing module in context for \"%wZ\" with \"%wZ\".\n",
                __FUNCTION__,
                &LoadContext->DllName,
                &ExistingModule->FullDllName);
    }

    RtlAcquireSRWLockExclusive(&LdrpModuleDatatableLock);

    ASSERT(LoadContext->Module->ReferenceCount <= 2);

    const PLDR_DDAG_NODE OldEntryNode = LoadContext->Module->DdagNode;

    LDRP_ASSERT_DDAG_NODE(OldEntryNode);

    ASSERT(!OldEntryNode->Dependencies.Tail);
    ASSERT(OldEntryNode->LoadCount == 1);

    const PLDR_DDAG_NODE ExistingDdagNode = ExistingModule->DdagNode;

    LDRP_ASSERT_DDAG_NODE(ExistingDdagNode);
    ASSERT(ExistingDdagNode->LoadCount >= 1);
    ASSERT(ExistingModule->ReferenceCount > 1 || LDRP_NODE_MODULE_PINNED(ExistingDdagNode, ExistingModule));

    LoadContext->Module = ExistingModule;
    OldEntryNode->LoadCount = 0u;

    if (ExistingDdagNode->LoadCount != LDR_LOADCOUNT_MAX)
        ++ExistingDdagNode->LoadCount;

    if (OldEntryNode->IncomingDependencies.Tail)
    {
        const PSINGLE_LIST_ENTRY NextEntry = OldEntryNode->IncomingDependencies.Tail->Next;

        if (NextEntry == OldEntryNode->IncomingDependencies.Tail)
            OldEntryNode->IncomingDependencies.Tail = NULL;
        else
            OldEntryNode->IncomingDependencies.Tail->Next = NextEntry->Next;

        LdrpDereferenceModule(ExistingModule);

        ASSERT(NextEntry);
        ASSERT(!OldEntryNode->IncomingDependencies.Tail);

        // The link from parent to OldEntryNode
        PLDRP_CSLIST_ENTRY Dependency = CONTAINING_RECORD(NextEntry, LDRP_CSLIST_ENTRY, IncomingDependenciesLink);

        LDRP_ASSERT_CSLIST_ENTRY(Dependency);

        ASSERT(Dependency->DependencyNode == OldEntryNode);

        const PLDR_DDAG_NODE DependingNode = Dependency->ParentNode;

        LDRP_ASSERT_DDAG_NODE(DependingNode);

        const PLDR_DATA_TABLE_ENTRY DependingModule = LDRP_FIRST_MODULE(DependingNode);

        LDRP_ASSERT_MODULE_ENTRY(DependingModule);
        LDRP_ASSERT_NODE_ENTRY(DependingNode, DependingModule);

        const PLDRP_LOAD_CONTEXT DependingContext = DependingModule->LoadContext;

        LDRP_ASSERT_LOAD_CONTEXT(DependingContext);

        // Do we already have a dependency link with replacement node?
        if (LdrpDependencyExist(DependingNode, ExistingDdagNode))
        {
            // Remove the Dependency from DependingNode->Dependencies list.
            LdrpRemoveDependencyEntry(&DependingNode->Dependencies, &Dependency->DependenciesLink);

            // Dereference ExistingModule node (the number of incoming dependencies has dropped)
            LdrpDecrementNodeLoadCountLockHeld(ExistingDdagNode, FALSE, NULL);

            // Destroy the link we no longer need
            LdrpHeapFree(0, Dependency);
            Dependency = NULL;
        }
        else
        {
            // No already existing dependency, add one. Reuse the link with the old (replaced) node.
            Dependency->DependencyNode = ExistingDdagNode;

            const PSINGLE_LIST_ENTRY Link = &Dependency->IncomingDependenciesLink;
            const PSINGLE_LIST_ENTRY Tail = ExistingDdagNode->IncomingDependencies.Tail;

            if (Tail)
                PushEntryList(Tail, Link);
            else
                Link->Next = Link;

            ExistingDdagNode->IncomingDependencies.Tail = Link;
        }

        if (DependingContext && (ExistingDdagNode->State >= LdrModulesMapped || !Dependency))
        {
            ASSERT(DependingModule->NodeModuleLink.Flink == &DependingNode->Modules);
            ASSERT(DependingNode->State < LdrModulesSnapping);
            ASSERT(DependingContext->UnmappedChildCount > 0);

            LdrpDecrementUnmappedChildCount(DependingContext, DependingModule, LoadContext, ExistingModule);
        }

        ASSERT(!OldEntryNode->IncomingDependencies.Tail);
    }

    ASSERT(!OldEntryNode->Dependencies.Tail);

    RtlReleaseSRWLockExclusive(&LdrpModuleDatatableLock);
}
