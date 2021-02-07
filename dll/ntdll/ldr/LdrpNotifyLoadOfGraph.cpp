#include <ldrp.h>

NTSTATUS
NTAPI
LdrpSendPostSnapNotifications(PLDR_DDAG_NODE Node)
{
    LDRP_ASSERT_DDAG_NODE(Node);

    // todo: AVrfDllLoadNotification
    // todo: LdrpSendDllNotifications
    // todo: iterate Modules in backwards order and do things for each module instead of just one

    PPEB Peb = NtCurrentPeb();
    const PLDR_DATA_TABLE_ENTRY Module = LDRP_FIRST_MODULE(Node);

    LDRP_ASSERT_MODULE_ENTRY(Module);
    LDRP_ASSERT_NODE_ENTRY(Node, Module);

    /* Notify Shim Engine */
    if (g_ShimsEnabled)
    {
        void (NTAPI * SE_DllLoaded)(PLDR_DATA_TABLE_ENTRY) = static_cast<decltype(SE_DllLoaded)>(
            RtlDecodeSystemPointer(g_pfnSE_DllLoaded)
        );
        SE_DllLoaded(Module);
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
        AVrfPageHeapDllNotification(Module);
    }

    /* Check if Application Verifier was enabled */
    if (Peb->NtGlobalFlag & FLG_APPLICATION_VERIFIER)
    {
        AVrfDllLoadNotification(Module);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
LdrpNotifyLoadOfGraph(PLDR_DDAG_NODE Node)
{
    LDRP_ASSERT_DDAG_NODE(Node);

    NTSTATUS Status = STATUS_SUCCESS;
    const PLDR_DATA_TABLE_ENTRY ParentLdrEntry = LDRP_FIRST_MODULE(Node);

    LDRP_ASSERT_MODULE_ENTRY(ParentLdrEntry);
    LDRP_ASSERT_NODE_ENTRY(Node, ParentLdrEntry);

    const PSINGLE_LIST_ENTRY ListHead = Node->Dependencies.Tail;
    for (PSINGLE_LIST_ENTRY ListEntry = ListHead ? ListHead->Next : NULL;
         ListEntry;
         ListEntry = ListEntry != ListHead ? ListEntry->Next : NULL)
    {
        const PLDRP_CSLIST_ENTRY DependencyEntry = CONTAINING_RECORD(ListEntry, LDRP_CSLIST_ENTRY, DependenciesLink);

        LDRP_ASSERT_CSLIST_ENTRY(DependencyEntry);

        const PLDR_DDAG_NODE DependencyNode = DependencyEntry->DependencyNode;

        LDRP_ASSERT_DDAG_NODE(DependencyNode);

        const PLDR_DATA_TABLE_ENTRY DependencyLdrEntry = LDRP_FIRST_MODULE(DependencyNode);

        LDRP_ASSERT_MODULE_ENTRY(DependencyLdrEntry);
        LDRP_ASSERT_NODE_ENTRY(DependencyNode, DependencyLdrEntry);

        switch (DependencyNode->State)
        {
            case LdrModulesInitError:
                DPRINT1("%s(" LDR_HEXPTR_FMT "[%wZ:%d]): dependency node " LDR_HEXPTR_FMT "[%wZ:%d] is in an failure state, aborting.\n",
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
                DPRINT1("%s(" LDR_HEXPTR_FMT "[%wZ:%d]): dependency node " LDR_HEXPTR_FMT "[%wZ:%d] is in an unexpected state, aborting.\n",
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

            DPRINT1("%s(" LDR_HEXPTR_FMT "[%wZ:%d]): LdrpSendPostSnapNotifications(...) -> 0x%08lX, rolling back.\n",
                    __FUNCTION__,
                    Node, &ParentLdrEntry->BaseDllName, Node->State,
                    Status);

            Node->State = LdrModulesCondensed;
        }
    }

    return Status;
}
