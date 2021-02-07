#include <ldrp.h>

void
NTAPI
LdrpFreeLoadContextOfNode(PLDR_DDAG_NODE Node, NTSTATUS *StatusResponse)
{
    LDRP_ASSERT_DDAG_NODE(Node);

    DPRINT1("LDR: %s(" LDR_HEXPTR_FMT ", " LDR_HEXPTR_FMT "[0x%08lX])\n", __FUNCTION__, Node, StatusResponse, StatusResponse ? *StatusResponse : 0);

    const PLDR_DATA_TABLE_ENTRY FirstModule = LDRP_FIRST_MODULE(Node);

    LDRP_ASSERT_MODULE_ENTRY(FirstModule);
    LDRP_ASSERT_NODE_ENTRY(Node, FirstModule);

    const PLDRP_LOAD_CONTEXT FirstModuleContext = FirstModule->LoadContext;

    if (!FirstModuleContext)
        return;

    LDRP_ASSERT_LOAD_CONTEXT(FirstModuleContext);

    // Easy way to check if this LoadContext is in the current load session which we are cleaning up after.
    if (FirstModuleContext->LoadStatus != StatusResponse)
        return;

    const PLIST_ENTRY Head = &Node->Modules;
    for (PLIST_ENTRY Current = Head->Flink; Current != Head; Current = Current->Flink)
    {
        const PLDR_DATA_TABLE_ENTRY Module = CONTAINING_RECORD(Current, LDR_DATA_TABLE_ENTRY, NodeModuleLink);

        LDRP_ASSERT_MODULE_ENTRY(Module);
        LDRP_ASSERT_NODE_ENTRY(Node, Module);

        LdrpFreeLoadContext(Module->LoadContext);
    }

    // Recurse into dependencies

    const PSINGLE_LIST_ENTRY ListHead = Node->Dependencies.Tail;
    for (PSINGLE_LIST_ENTRY ListEntry = ListHead ? ListHead->Next : NULL;
         ListEntry;
         ListEntry = ListEntry != ListHead ? ListEntry->Next : NULL)
    {
        const PLDRP_CSLIST_ENTRY DependencyEntry = CONTAINING_RECORD(ListEntry, LDRP_CSLIST_ENTRY, DependenciesLink);

        LDRP_ASSERT_CSLIST_ENTRY(DependencyEntry);

        ASSERT(DependencyEntry->ParentNode == Node);

        LdrpFreeLoadContextOfNode(DependencyEntry->DependencyNode, StatusResponse);
    }
}
