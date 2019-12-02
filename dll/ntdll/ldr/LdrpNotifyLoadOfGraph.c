#include <ldrp.h>

NTSTATUS
NTAPI
LdrpSendPostSnapNotifications(PLDR_DDAG_NODE Node)
{
    // todo: AVrfDllLoadNotification
    // todo: LdrpSendDllNotifications
    // todo: iterate Modules in backwards order and do things for each module instead of just one
    PPEB Peb = NtCurrentPeb();
    const PLDR_DATA_TABLE_ENTRY LdrEntry = CONTAINING_RECORD(Node->Modules.Flink, LDR_DATA_TABLE_ENTRY, NodeModuleLink);

    /* Notify Shim Engine */
    if (g_ShimsEnabled)
    {
        VOID(NTAPI * SE_DllLoaded)(PLDR_DATA_TABLE_ENTRY) = RtlDecodeSystemPointer(g_pfnSE_DllLoaded);
        SE_DllLoaded(LdrEntry);
    }

    /* Check for Per-DLL Heap Tagging */
    if (Peb->NtGlobalFlag & FLG_HEAP_ENABLE_TAG_BY_DLL)
    {
        /* FIXME */
        DPRINT1("We don't support Per-DLL Heap Tagging yet!\n");
    }


    /* Check if Page Heap was enabled */
    if (Peb->NtGlobalFlag & FLG_HEAP_PAGE_ALLOCS)
    {
        /* Initialize target DLL */
        AVrfPageHeapDllNotification(LdrEntry);
    }

    /* Check if Application Verifier was enabled */
    if (Peb->NtGlobalFlag & FLG_APPLICATION_VERIFIER)
    {
        AVrfDllLoadNotification(LdrEntry);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
LdrpNotifyLoadOfGraph(PLDR_DDAG_NODE Node)
{
    NTSTATUS Status = STATUS_SUCCESS;
    const PLDR_DATA_TABLE_ENTRY ParentLdrEntry = CONTAINING_RECORD(Node->Modules.Flink, LDR_DATA_TABLE_ENTRY, NodeModuleLink);

    const PSINGLE_LIST_ENTRY ListHead = Node->Dependencies.Tail;
    for (PSINGLE_LIST_ENTRY ListEntry = ListHead ? ListHead->Next : NULL;
         ListEntry;
         ListEntry = (ListEntry != ListHead) ? ListEntry->Next : NULL)
    {
        const PLDRP_CSLIST_ENTRY DependencyEntry = CONTAINING_RECORD(ListEntry, LDRP_CSLIST_ENTRY, DependenciesLink);
        const PLDR_DDAG_NODE DependencyNode = DependencyEntry->DependencyNode;
        const PLDR_DATA_TABLE_ENTRY DependencyLdrEntry = CONTAINING_RECORD(DependencyNode->Modules.Flink, LDR_DATA_TABLE_ENTRY, NodeModuleLink);

        switch (DependencyNode->State)
        {
            case LdrModulesInitError:
                DPRINT1("%s(%p[%wZ:%d]): dependency node %p[%wZ:%d] is in an failure state, aborting.\n",
                        __FUNCTION__,
                        Node, &ParentLdrEntry->BaseDllName, Node->State,
                        DependencyNode, &DependencyLdrEntry->BaseDllName, DependencyNode->State);
                Status = STATUS_DLL_INIT_FAILED;
                goto Quickie;
            case LdrModulesCondensed:
                Status = LdrpNotifyLoadOfGraph(DependencyNode);
                if (!NT_SUCCESS(Status))
                    goto Quickie;
                continue;
            case LdrModulesReadyToInit:
            case LdrModulesInitializing:
            case LdrModulesReadyToRun:
                Status = STATUS_SUCCESS;
                continue;
            default:
                DPRINT1("%s(%p[%wZ:%d]): dependency node %p[%wZ:%d] is in an unexpected state, aborting.\n",
                        __FUNCTION__,
                        Node, &ParentLdrEntry->BaseDllName, Node->State,
                        DependencyNode, &DependencyLdrEntry->BaseDllName, DependencyNode->State);
                Status = STATUS_INTERNAL_ERROR;
                goto Quickie;
        }
    }

Quickie:
    if (NT_SUCCESS(Status))
    {
        Node->State = LdrModulesReadyToInit;
        Status = LdrpSendPostSnapNotifications(Node);
        if (!NT_SUCCESS(Status))
        {
            // Rollback

            DPRINT1("%s(%p[%wZ:%d]): LdrpSendPostSnapNotifications(...) -> 0x%08lX, rolling back.\n",
                    __FUNCTION__,
                    Node, &ParentLdrEntry->BaseDllName, Node->State,
                    Status);

            Node->State = LdrModulesCondensed;
        }
    }

    return Status;
}
