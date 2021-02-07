#include <ldrp.h>

void
LdrpLoadDllInternal(IN PUNICODE_STRING DllName,
                    IN PLDRP_PATH_SEARCH_CONTEXT PathSearchContext,
                    IN LDRP_LOAD_CONTEXT_FLAGS LoadContextFlags,
                    IN LDR_DLL_LOAD_REASON LoadReason,
                    IN PLDR_DATA_TABLE_ENTRY ParentEntry,
                    IN PLDR_DATA_TABLE_ENTRY ForwarderSource OPTIONAL,
                    OUT PLDR_DATA_TABLE_ENTRY* OutputLdrEntry,
                    OUT NTSTATUS* OutStatus)
{
    PTEB Teb = NtCurrentTeb();
    PLDR_DATA_TABLE_ENTRY Module = NULL;

    ASSERT(DllName);
    ASSERT(OutputLdrEntry);
    ASSERT(OutStatus);

    /* Show debug message */
    if (LdrpDebugFlags.LogTrace)
    {
        DPRINT1("LDR: %s(\"%wZ\", ...)\n", __FUNCTION__, DllName);
    }

    *OutputLdrEntry = NULL;

    NTSTATUS Status = LdrpFastpthReloadedDll(DllName, LoadContextFlags, ForwarderSource, OutputLdrEntry);

    if (!NT_SUCCESS(Status))
    {
        const BOOLEAN LoadOwner = Teb->LoadOwner;

        if (!LoadOwner)
            LdrpDrainWorkQueue(LoadComplete);

        if (!ForwarderSource || LoadOwner || ForwarderSource->DdagNode->LoadCount)
        {
            Status = LdrpFindOrPrepareLoadingModule(DllName,
                                                    PathSearchContext,
                                                    LoadContextFlags,
                                                    LoadReason,
                                                    ParentEntry,
                                                    &Module,
                                                    OutStatus);

            if (Status == STATUS_DLL_NOT_FOUND)
            {
                LdrpProcessWork(Module->LoadContext, StandaloneRequest);
            }
            else if (!NT_SUCCESS(Status) && Status != STATUS_RETRY)
            {
                DPRINT1("LDR: LdrpFindOrPrepareLoadingModule(\"%wZ\", ...) -> 0x%08lX\n",
                        DllName, Status);
                *OutStatus = Status;
            }
        }
        else
        {
            *OutStatus = STATUS_DLL_NOT_FOUND;
        }

        LdrpDrainWorkQueue(WorkComplete);

        if (Module)
        {
            // We might have replaced Entry during mapping (e.g. found already mapped entry)
            const PLDR_DATA_TABLE_ENTRY ReplacedEntry = LdrpHandleReplacedModule(Module);
            if (ReplacedEntry != Module)
            {
                LdrpFreeReplacedModule(Module);
                Module = ReplacedEntry;
            }

            *OutputLdrEntry = Module;

            if (Module->LoadContext)
                LdrpCondenseGraph(Module->DdagNode);

            if (NT_SUCCESS(*OutStatus))
            {
                *OutStatus = LdrpPrepareModuleForExecution(Module, OutStatus);

                if (NT_SUCCESS(*OutStatus))
                {
                    *OutStatus = LdrpBuildForwarderLink(ForwarderSource, Module);

                    if (NT_SUCCESS(*OutStatus) && !LdrInitState)
                    {
                        LdrpPinModule(Module);
                    }
                }
            }

            LdrpFreeLoadContextOfNode(Module->DdagNode, OutStatus);

            if (!NT_SUCCESS(*OutStatus))
            {
                *OutputLdrEntry = NULL;
                LdrpDecrementModuleLoadCountEx(Module, FALSE);
                LdrpDereferenceModule(Module);
            }
        }
        else
        {
            *OutStatus = STATUS_NO_MEMORY;
        }

        if (!LoadOwner)
            LdrpDropLastInProgressCount();
    }
    else
    {
        *OutStatus = Status;
    }

    if (LdrpDebugFlags.LogTrace)
    {
        DPRINT1("LDR: %s(\"%wZ\", ..., %d, [%wZ], [%wZ], ...) -> 0x%08lX\n",
                __FUNCTION__,
                DllName,
                LoadReason,
                ParentEntry ? &ParentEntry->BaseDllName : NULL,
                ForwarderSource ? &ForwarderSource->BaseDllName : NULL,
                *OutStatus);
    }
}
