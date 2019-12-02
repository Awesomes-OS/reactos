#include <ldrp.h>

NTSTATUS
NTAPI
LdrpInitializeGraphRecurse(IN PLDR_DDAG_NODE DdagNode,
                           IN NTSTATUS* StatusResponse,
                           IN OUT BOOLEAN* HasInitializing)
{
    if (DdagNode->State == LdrModulesInitError)
        return STATUS_DLL_INIT_FAILED;

    NTSTATUS Status;

    const PSINGLE_LIST_ENTRY ListHead = DdagNode->Dependencies.Tail;

    if (!ListHead)
        goto SkipLoop;

    BOOLEAN HasInitializingDependency = FALSE;

    for (PSINGLE_LIST_ENTRY ListEntry = ListHead ? ListHead->Next : NULL;
         ListEntry;
         ListEntry = (ListEntry != ListHead) ? ListEntry->Next : NULL)
    {
        const PLDRP_CSLIST_ENTRY DependencyEntry = CONTAINING_RECORD(ListEntry, LDRP_CSLIST_ENTRY, DependenciesLink);
        const PLDR_DDAG_NODE Dependency = DependencyEntry->DependencyNode;

        switch (Dependency->State)
        {
            case LdrModulesReadyToInit:
            {
                Status = LdrpInitializeGraphRecurse(Dependency, StatusResponse, &HasInitializingDependency);
                if (!NT_SUCCESS(Status))
                    goto Quickie;
                break;
            }
            case LdrModulesInitializing:
            {
                HasInitializingDependency = TRUE;
                break;
            }
            case LdrModulesInitError:
            {
                Status = STATUS_DLL_INIT_FAILED;
                goto Quickie;
            }
            default:
            {
                break;
            }
        }
    }

    if (HasInitializingDependency)
    {
        const PLDR_DATA_TABLE_ENTRY LdrEntry = CONTAINING_RECORD(DdagNode, LDR_DATA_TABLE_ENTRY, NodeModuleLink);
        const PLDRP_LOAD_CONTEXT LoadContext = LdrEntry->LoadContext;

        *HasInitializing = TRUE;

        if (LoadContext)
        {
            if (StatusResponse != LoadContext->StatusResponse)
            {
                // This is not what we are here for. That's different load session.
                // Skip graph node initialization.
                Status = STATUS_SUCCESS;
                goto Quickie;
            }
        }
    }

SkipLoop:

    Status = LdrpInitializeNode(DdagNode);

Quickie:
    if (!NT_SUCCESS(Status))
        DdagNode->State = LdrModulesInitError;

    return Status;
}
