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

    const auto* const ListHead = DdagNode->Dependencies.Tail;

    BOOLEAN HasInitializingDependency = FALSE;

    if (!ListHead)
        goto SkipLoop;

    for (const auto* ListEntry = ListHead ? ListHead->Next : NULL;
         ListEntry;
         ListEntry = (ListEntry != ListHead) ? ListEntry->Next : NULL)
    {
        const auto* const DependencyEntry = CONTAINING_RECORD(ListEntry, LDRP_CSLIST_ENTRY, DependenciesLink);

        LDRP_ASSERT_CSLIST_ENTRY(DependencyEntry);

        auto* const Dependency = DependencyEntry->DependencyNode;

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
        const auto* const Module = LDRP_FIRST_MODULE(DdagNode);

        LDRP_ASSERT_MODULE_ENTRY(Module);
        LDRP_ASSERT_NODE_ENTRY(DdagNode, Module);

        const auto* const LoadContext = Module->LoadContext;

        LDRP_ASSERT_LOAD_CONTEXT_OPTIONAL(LoadContext);

        *HasInitializing = TRUE;

        if (LoadContext)
        {
            if (StatusResponse != LoadContext->LoadStatus)
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
