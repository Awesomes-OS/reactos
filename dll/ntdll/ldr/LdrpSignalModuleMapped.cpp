#include <ldrp.h>

void
NTAPI
LdrpDecrementUnmappedChildCount(IN PLDRP_LOAD_CONTEXT ParentContext,
                                IN PLDR_DATA_TABLE_ENTRY ParentModule,
                                IN PLDRP_LOAD_CONTEXT ChildContext,
                                IN PLDR_DATA_TABLE_ENTRY ChildModule)
{
    ASSERT(ParentContext || ParentModule);

    if (ParentContext && !ParentModule)
        ParentModule = ParentContext->Module;
    else if (!ParentContext && ParentModule)
        ParentContext = ParentModule->LoadContext;

    if (ChildContext && !ChildModule)
        ChildModule = ChildContext->Module;
    else if (!ChildContext && ChildModule)
        ChildContext = ChildModule->LoadContext;

    LDRP_ASSERT_LOAD_CONTEXT(ParentContext);
    LDRP_ASSERT_MODULE_ENTRY(ParentModule);
    LDRP_ASSERT_LOAD_CONTEXT_OPTIONAL(ChildContext);
    LDRP_ASSERT_MODULE_ENTRY_OPTIONAL(ChildModule);

    const PLDR_DDAG_NODE ParentNode = ParentModule->DdagNode;
    const PLDR_DDAG_NODE ChildNode = ChildModule ? ChildModule->DdagNode : NULL;

    LDRP_ASSERT_DDAG_NODE(ParentNode);
    LDRP_ASSERT_DDAG_NODE_OPTIONAL(ChildNode);

    DPRINT1("LDR: %s([%wZ]:unmapped=%lu, [%wZ:%d], [%wZ], [%wZ:%d]): dependency mapped\n",
            __FUNCTION__,
            &ParentContext->DllName,
            ParentContext->UnmappedChildCount,
            &ParentModule->BaseDllName,
            ParentNode->State,
            ChildContext ? &ChildContext->DllName : NULL,
            ChildModule ? &ChildModule->BaseDllName : NULL,
            ChildNode ? ChildNode->State : 0);

    ASSERT(ParentNode->State == LdrModulesMapped || ParentNode->State == LdrModulesWaitingForDependencies);
    ASSERT(ParentContext->UnmappedChildCount);

    --ParentContext->UnmappedChildCount;

    if (!ParentContext->UnmappedChildCount)
    {
        ParentModule->DdagNode->State = LdrModulesSnapping;
        LdrpQueueWork(ParentContext);
    }
}

void
NTAPI
LdrpSignalModuleMapped(IN PLDR_DATA_TABLE_ENTRY Module)
{
    LDRP_ASSERT_MODULE_ENTRY(Module);

    const PSINGLE_LIST_ENTRY ListHead = Module->DdagNode->IncomingDependencies.Tail;
    for (PSINGLE_LIST_ENTRY ListEntry = ListHead ? ListHead->Next : NULL;
         ListEntry;
         ListEntry = ListEntry != ListHead ? ListEntry->Next : NULL)
    {
        const PLDRP_CSLIST_ENTRY DependencyEntry = CONTAINING_RECORD(ListEntry, LDRP_CSLIST_ENTRY,
                                                                     IncomingDependenciesLink);

        LDRP_ASSERT_CSLIST_ENTRY(DependencyEntry);

        ASSERT(DependencyEntry->DependencyNode == Module->DdagNode);

        const PLDR_DDAG_NODE ParentNode = DependencyEntry->ParentNode;

        LDRP_ASSERT_DDAG_NODE(ParentNode);

        const PLDR_DATA_TABLE_ENTRY ParentModule = LDRP_FIRST_MODULE(ParentNode);

        LDRP_ASSERT_MODULE_ENTRY(ParentModule);
        LDRP_ASSERT_NODE_ENTRY(ParentNode, ParentModule);

        ASSERT(ParentModule->NodeModuleLink.Flink == &ParentNode->Modules);

        LDRP_ASSERT_LOAD_CONTEXT(ParentModule->LoadContext);

        LdrpDecrementUnmappedChildCount(NULL, ParentModule, NULL, Module);
    }
}
