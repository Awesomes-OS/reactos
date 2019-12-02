#include <ldrp.h>

void
NTAPI
LdrpFreeLoadContextOfNode(PLDR_DDAG_NODE Node, NTSTATUS *StatusResponse)
{
    const PLDR_DATA_TABLE_ENTRY LdrEntry = CONTAINING_RECORD(Node, LDR_DATA_TABLE_ENTRY, NodeModuleLink);

    const PLDRP_LOAD_CONTEXT LoadContext = LdrEntry->LoadContext;

    if (!LoadContext)
        return;

    // Easy way to check if this LoadContext is in the current load session which we are cleaning up after.
    if (LoadContext->StatusResponse != StatusResponse)
        return;

    PLDR_DATA_TABLE_ENTRY DependencyEntry = LdrEntry;
    PLDR_DDAG_NODE DependencyNode = Node;
    do
    {
        LdrpFreeLoadContext(DependencyEntry->LoadContext);

        DependencyNode = CONTAINING_RECORD(DependencyEntry->NodeModuleLink.Flink, LDR_DDAG_NODE, Modules);
        DependencyEntry = CONTAINING_RECORD(DependencyNode, LDR_DATA_TABLE_ENTRY, NodeModuleLink);
    }
    while (DependencyNode != Node);

    const PSINGLE_LIST_ENTRY ListHead = Node->Dependencies.Tail;
    for (PSINGLE_LIST_ENTRY ListEntry = ListHead ? ListHead->Next : NULL;
         ListEntry;
         ListEntry = (ListEntry != ListHead) ? ListEntry->Next : NULL)
    {
        const PLDRP_CSLIST_ENTRY DependencyEntry = CONTAINING_RECORD(ListEntry, LDRP_CSLIST_ENTRY, DependenciesLink);
        LdrpFreeLoadContextOfNode(DependencyEntry->DependencyNode, StatusResponse);
    }
}