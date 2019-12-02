#include <ldrp.h>
#include <reactos/ldrp.h>

void
NTAPI
LdrpProcessDetachNode(IN PLDR_DDAG_NODE Node)
{
    LIST_ENTRY* Head = &Node->Modules;
    for (LIST_ENTRY* Current = Head->Flink; Current != Head; Current = Current->Flink)
    {
        const PLDR_DDAG_NODE CurrentNode = CONTAINING_RECORD(Current, LDR_DDAG_NODE, Modules);
        const PLDR_DATA_TABLE_ENTRY CurrentEntry = CONTAINING_RECORD(CurrentNode, LDR_DATA_TABLE_ENTRY, NodeModuleLink);

        CurrentEntry->ObsoleteLoadCount = 0;

        LdrpRecordUnloadEvent(CurrentEntry);

        /* Unlink it */
        RemoveEntryList(&CurrentEntry->InInitializationOrderLinks);

        // Call the entrypoint to notify about the situation
        if (CurrentEntry->EntryPoint && CurrentEntry->ProcessAttachCalled)
        {
            RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_EXTENDED ActCtx;

            /* Show message */
            if (ShowSnaps)
            {
                DPRINT1("Uninitializing [%wZ] (LoadCount %lx) (EntryPoint @ %p)\n",
                        &CurrentEntry->BaseDllName,
                        CurrentEntry->DdagNode->LoadCount,
                        CurrentEntry->EntryPoint);
            }


            /* Set up the Act Ctx */
            ActCtx.Size = sizeof(ActCtx);
            ActCtx.Format = RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_FORMAT_WHISTLER;
            RtlZeroMemory(&ActCtx.Frame, sizeof(RTL_ACTIVATION_CONTEXT_STACK_FRAME));

            /* Activate the ActCtx */
            RtlActivateActivationContextUnsafeFast(&ActCtx,
                                                   CurrentEntry->EntryPointActivationContext);


            if (CurrentEntry->TlsIndex)
            {
                _SEH2_TRY
                {
                    /* Do TLS callbacks */
                    LdrpCallTlsInitializers(CurrentEntry, DLL_PROCESS_DETACH);
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    /* Do nothing */
                }
                _SEH2_END;
            }

            /* Call the entrypoint */
            _SEH2_TRY
            {
                LdrpCallInitRoutine(CurrentEntry->EntryPoint,
                                    CurrentEntry->DllBase,
                                    DLL_PROCESS_DETACH,
                                    NULL);
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                DPRINT1("WARNING: Exception 0x%x during LdrpCallInitRoutine(DLL_PROCESS_DETACH) for %wZ\n",
                        _SEH2_GetExceptionCode(), &CurrentEntry->BaseDllName);
            }
            _SEH2_END;

            /* Release the context */
            RtlDeactivateActivationContextUnsafeFast(&ActCtx);
        }
    }
}

NTSTATUS
NTAPI
LdrpUnloadNode(IN PLDR_DDAG_NODE Node)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PPEB Peb = NtCurrentPeb();

    /* Get the entry */
    PLDR_DATA_TABLE_ENTRY LdrEntry = CONTAINING_RECORD(Node,
                                                       LDR_DATA_TABLE_ENTRY,
                                                       InInitializationOrderLinks);


    // Notify everyone interested about the situation
    if (Node->State == LdrModulesReadyToRun || Node->State == LdrModulesInitError)
    {
        VOID (NTAPI * SE_DllUnloaded)(PVOID) = NULL;

        Node->State = LdrModulesUnloading;

        LdrpProcessDetachNode(Node);

        // Get the Shim Engine hook for later calls
        if (g_ShimsEnabled)
        {
            SE_DllUnloaded = RtlDecodeSystemPointer(g_pfnSE_DllUnloaded);
        }

        LIST_ENTRY* Head = &Node->Modules;
        for (LIST_ENTRY* Current = Head->Flink; Current != Head; Current = Current->Flink)
        {
            const PLDR_DDAG_NODE CurrentNode = CONTAINING_RECORD(Current, LDR_DDAG_NODE, Modules);
            const PLDR_DATA_TABLE_ENTRY CurrentEntry = CONTAINING_RECORD(
                CurrentNode, LDR_DATA_TABLE_ENTRY, NodeModuleLink);

            /* Call Shim Engine and notify */
            if (SE_DllUnloaded)
            {
                SE_DllUnloaded(LdrEntry);
            }

            /* Notify Application Verifier */
            if (Peb->NtGlobalFlag & FLG_APPLICATION_VERIFIER)
            {
                AVrfDllUnloadNotification(LdrEntry);
            }

            /* Show message */
            if (ShowSnaps)
            {
                DPRINT1("LDR: Unmapping [%ws]\n", LdrEntry->BaseDllName.Buffer);
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


        RtlAcquireSRWLockExclusive(&LdrpModuleDatatableLock);

        const PLDRP_CSLIST_ENTRY DependencyEntry = CONTAINING_RECORD(DependencyListEntry, LDRP_CSLIST_ENTRY,
                                                                     DependenciesLink);
        const PLDR_DDAG_NODE DependencyNode = DependencyEntry->DependencyNode;

        BOOLEAN BecameOrphan = FALSE;

        LdrpRemoveDependencyEntry(&DependencyNode->IncomingDependencies, &DependencyEntry->IncomingDependenciesLink);

        LdrpDecrementNodeLoadCountLockHeld(DependencyNode, FALSE, &BecameOrphan);

        RtlReleaseSRWLockExclusive(&LdrpModuleDatatableLock);

        if (BecameOrphan)
            LdrpUnloadNode(DependencyNode);

        LdrpFreeHeap(0, DependencyEntry);
    }

    Node->State = LdrModulesUnloaded;

    {
        LIST_ENTRY* ModulesHead = &Node->Modules;
        for (LIST_ENTRY* Current = ModulesHead->Flink; Current != ModulesHead; Current = Current->Flink)
        {
            const PLDR_DDAG_NODE CurrentNode = CONTAINING_RECORD(Current, LDR_DDAG_NODE, Modules);
            const PLDR_DATA_TABLE_ENTRY CurrentEntry = CONTAINING_RECORD(
                CurrentNode, LDR_DATA_TABLE_ENTRY, NodeModuleLink);

            RtlAcquireSRWLockExclusive(&LdrpModuleDatatableLock);

            if (CurrentEntry->InLegacyLists)
            {
                // Do not remove from InInitializationOrderLinks, it is being iterated later

                RemoveEntryList(&CurrentEntry->InLoadOrderLinks);
                RemoveEntryList(&CurrentEntry->InMemoryOrderLinks);
                RemoveEntryList(&CurrentEntry->HashLinks);
            }

            RtlReleaseSRWLockExclusive(&LdrpModuleDatatableLock);

            LdrpDereferenceModule(CurrentEntry);
        }
    }

    return Status;
}
