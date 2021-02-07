#include <ldrp.h>
#include <reactos/ldrp.h>

void
NTAPI
LdrpProcessDetachNode(IN PLDR_DDAG_NODE Node)
{
    LDRP_ASSERT_DDAG_NODE(Node);

    const PLIST_ENTRY Head = &Node->Modules;
    for (LIST_ENTRY* Current = Head->Flink; Current != Head; Current = Current->Flink)
    {
        const PLDR_DATA_TABLE_ENTRY CurrentEntry = CONTAINING_RECORD(Current, LDR_DATA_TABLE_ENTRY, NodeModuleLink);

        LDRP_ASSERT_MODULE_ENTRY(CurrentEntry);
        LDRP_ASSERT_NODE_ENTRY(Node, CurrentEntry);

        CurrentEntry->ObsoleteLoadCount = 0;

        LdrpRecordUnloadEvent(CurrentEntry);

        /* Unlink it */
        RemoveEntryList(&CurrentEntry->InInitializationOrderLinks);

        // Call the entrypoint to notify about the situation
        if (!CurrentEntry->EntryPoint || !CurrentEntry->ProcessAttachCalled)
            continue;

        RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_EXTENDED ActCtx;

        /* Show message */
        if (LdrpDebugFlags.LogTrace)
        {
            DPRINT1("Uninitializing [%wZ] (LoadCount %lx) (EntryPoint @ " LDR_HEXPTR_FMT ")\n",
                    &CurrentEntry->BaseDllName,
                    CurrentEntry->DdagNode->LoadCount,
                    CurrentEntry->EntryPoint);
        }


        /* Set up the Act Ctx */
        ActCtx.Size = sizeof(ActCtx);
        ActCtx.Format = RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_FORMAT_WHISTLER;
        RtlZeroMemory(&ActCtx.Frame, sizeof(RTL_ACTIVATION_CONTEXT_STACK_FRAME));

        /* Activate the ActCtx */
        RtlActivateActivationContextUnsafeFast(&ActCtx, CurrentEntry->EntryPointActivationContext);


        if (CurrentEntry->TlsIndex)
        {
            __try
            {
                /* Do TLS callbacks */
                LdrpCallTlsInitializers(CurrentEntry, DLL_PROCESS_DETACH);
            }
            __except(LdrpDebugExceptionFilter(GetExceptionCode(), GetExceptionInformation()))
            {
                /* Do nothing */
            }
        }

        /* Call the entrypoint */
        __try
        {
            LdrpCallInitRoutine((PDLL_INIT_ROUTINE)CurrentEntry->EntryPoint,
                                CurrentEntry->DllBase,
                                DLL_PROCESS_DETACH,
                                NULL);
        }
        __except(LdrpDebugExceptionFilter(GetExceptionCode(), GetExceptionInformation()))
        {
            DPRINT1("WARNING: Exception 0x%08lX during LdrpCallInitRoutine(DLL_PROCESS_DETACH) for %wZ\n",
                    GetExceptionCode(), &CurrentEntry->BaseDllName);
        }

        /* Release the context */
        RtlDeactivateActivationContextUnsafeFast(&ActCtx);
    }
}

void
LdrpRemoveDataTableEntry(const PLDR_DATA_TABLE_ENTRY Module)
{
    if (!Module->InLegacyLists)
        return;

    // Do not remove from InInitializationOrderLinks, it is being iterated later

    RemoveEntryList(&Module->InLoadOrderLinks);
    RemoveEntryList(&Module->InMemoryOrderLinks);
    RemoveEntryList(&Module->HashLinks);
    Module->InLegacyLists = FALSE;
}

NTSTATUS
NTAPI
LdrpUnloadNode(IN PLDR_DDAG_NODE Node)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PPEB Peb = NtCurrentPeb();

    LDRP_ASSERT_DDAG_NODE(Node);

    ASSERT(LdrpWorkInProgress == 1);
    ASSERT(!Node->LoadCount);
    ASSERT(!Node->IncomingDependencies.Tail);

    const PLIST_ENTRY ModulesHead = &Node->Modules;

    const BOOLEAN NotifyProcessDetach = Node->State == LdrModulesReadyToRun || Node->State == LdrModulesInitError;

    // Notify everyone interested about the situation
    if (NotifyProcessDetach)
    {
        Node->State = LdrModulesUnloading;

        LdrpProcessDetachNode(Node);
    }

    if (NotifyProcessDetach || Node->State == LdrModulesReadyToInit)
    {
        void (NTAPI *SE_DllUnloaded)(PVOID) = NULL;

        // Get the Shim Engine hook for later calls
        if (g_ShimsEnabled)
        {
            SE_DllUnloaded = static_cast<decltype(SE_DllUnloaded)>(RtlDecodeSystemPointer(g_pfnSE_DllUnloaded));
        }

        for (LIST_ENTRY* Current = ModulesHead->Flink; Current != ModulesHead; Current = Current->Flink)
        {
            const PLDR_DATA_TABLE_ENTRY CurrentEntry = CONTAINING_RECORD(Current, LDR_DATA_TABLE_ENTRY, NodeModuleLink);

            LDRP_ASSERT_MODULE_ENTRY(CurrentEntry);
            LDRP_ASSERT_NODE_ENTRY(Node, CurrentEntry);

            if (CurrentEntry->LoadNotificationsSent)
            {
                // todo: LdrpSendDllNotifications

                /* Call Shim Engine and notify */
                if (SE_DllUnloaded)
                {
                    SE_DllUnloaded(CurrentEntry);
                }

                /* Notify Application Verifier */
                if (Peb->NtGlobalFlag & FLG_APPLICATION_VERIFIER)
                {
                    AVrfDllUnloadNotification(CurrentEntry);
                }
            }

            /* Show message */
            if (LdrpDebugFlags.LogTrace)
            {
                DPRINT1("LDR: Unmapping [%ws]\n", CurrentEntry->BaseDllName.Buffer);
            }

            /* Unload the alternate resource module, if any */
            LdrUnloadAlternateResourceModule(CurrentEntry->DllBase);
        }
    }

    while (Node->Dependencies.Tail)
    {
        const PSINGLE_LIST_ENTRY DependencyListEntry = Node->Dependencies.Tail->Next;

        // Are there any other dependencies?
        if (DependencyListEntry == Node->Dependencies.Tail)
        {
            // The loop consists of just one dependency entry. Kill the whole list.
            Node->Dependencies.Tail = NULL;
        }
        else
        {
            // Yep, there are. Drop a single tail entry.
            Node->Dependencies.Tail->Next = DependencyListEntry->Next;
        }

        if (!DependencyListEntry)
            break;


        BOOLEAN BecameOrphan = FALSE;

        RtlAcquireSRWLockExclusive(&LdrpModuleDatatableLock);

        const PLDRP_CSLIST_ENTRY DependencyEntry = CONTAINING_RECORD(DependencyListEntry, LDRP_CSLIST_ENTRY,
                                                                     DependenciesLink);

        LDRP_ASSERT_CSLIST_ENTRY(DependencyEntry);

        ASSERT(DependencyEntry->ParentNode == Node);

        const PLDR_DDAG_NODE DependencyNode = DependencyEntry->DependencyNode;

        LDRP_ASSERT_DDAG_NODE(DependencyNode);

        LdrpRemoveDependencyEntry(&DependencyNode->IncomingDependencies, &DependencyEntry->IncomingDependenciesLink);

        LdrpDecrementNodeLoadCountLockHeld(DependencyNode, FALSE, &BecameOrphan);

        RtlReleaseSRWLockExclusive(&LdrpModuleDatatableLock);

        if (BecameOrphan)
            LdrpUnloadNode(DependencyNode);

        LDRP_ASSERT_CSLIST_ENTRY(DependencyEntry);

        LdrpFreeHeap(0, DependencyEntry);
    }

    Node->State = LdrModulesUnloaded;

    LDRP_ASSERT_DDAG_NODE(Node);

    for (LIST_ENTRY* Current = ModulesHead->Flink; Current != ModulesHead;)
    {
        const PLDR_DATA_TABLE_ENTRY CurrentEntry = CONTAINING_RECORD(Current, LDR_DATA_TABLE_ENTRY, NodeModuleLink);

        Current = Current->Flink;

        LDRP_ASSERT_DDAG_NODE(Node);
        LDRP_ASSERT_MODULE_ENTRY(CurrentEntry);
        LDRP_ASSERT_NODE_ENTRY(Node, CurrentEntry);

        ASSERT(!CurrentEntry->MarkedForRemoval);
        CurrentEntry->MarkedForRemoval = TRUE;

        RtlAcquireSRWLockExclusive(&LdrpModuleDatatableLock);

        LdrpRemoveDataTableEntry(CurrentEntry);

        RtlReleaseSRWLockExclusive(&LdrpModuleDatatableLock);

        LDRP_ASSERT_DDAG_NODE(Node);

        LdrpDereferenceModule(CurrentEntry);
    }

    return Status;
}
