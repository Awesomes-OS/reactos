#include <ldrp.h>

void
NTAPI
LdrpLoadDllInternal(IN PUNICODE_STRING DllName,
                    IN PLDRP_PATH_SEARCH_CONTEXT PathSearchContext,
                    IN LDRP_LOAD_CONTEXT_FLAGS LoadContextFlags,
                    IN LDR_DLL_LOAD_REASON LoadReason,
                    IN PLDR_DATA_TABLE_ENTRY ParentEntry,
                    IN PLDR_DATA_TABLE_ENTRY ForwarderSource OPTIONAL,
                    OUT PLDR_DATA_TABLE_ENTRY* OutputLdrEntry,
                    OUT NTSTATUS* OutStatus)
{
    PLDR_DATA_TABLE_ENTRY LdrEntry = NULL;

    /* Show debug message */
    if (ShowSnaps)
    {
        DPRINT1("LDR: %s(\"%wZ\", ...)\n", __FUNCTION__, DllName);
    }

    *OutputLdrEntry = NULL;

    NTSTATUS Status = LdrpFastpthReloadedDll(DllName, LoadContextFlags, ForwarderSource, OutputLdrEntry);

    if (!NT_SUCCESS(Status))
    {
        // todo: check if unloading...
        LdrpDrainWorkQueue();

        if (!ForwarderSource || ForwarderSource->DdagNode->LoadCount)
        {
            Status = LdrpFindOrPrepareLoadingModule(DllName,
                                                    PathSearchContext,
                                                    LoadContextFlags,
                                                    LoadReason,
                                                    ParentEntry,
                                                    &LdrEntry,
                                                    OutStatus);

            if (Status == STATUS_DLL_NOT_FOUND)
            {
                LdrpProcessWork(LdrEntry->LoadContext);
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

        LdrpDrainWorkQueue();

        if (LdrEntry)
        {
            // We might have replaced Entry during mapping (e.g. found already mapped entry)
            const PLDR_DATA_TABLE_ENTRY ReplacedEntry = LdrpHandleReplacedModule(LdrEntry);
            if (ReplacedEntry != LdrEntry)
            {
                LdrpFreeReplacedModule(LdrEntry);
                LdrEntry = ReplacedEntry;
            }

            *OutputLdrEntry = LdrEntry;

            if (LdrEntry->LoadContext)
                LdrpCondenseGraph(LdrEntry->DdagNode);

            if (NT_SUCCESS(*OutStatus))
            {
                *OutStatus = LdrpPrepareModuleForExecution(LdrEntry, OutStatus);

                if (NT_SUCCESS(*OutStatus))
                {
                    *OutStatus = LdrpBuildForwarderLink(ForwarderSource, LdrEntry);

                    if (NT_SUCCESS(*OutStatus) && LdrpInLdrInit)
                    {
                        LdrpPinModule(LdrEntry);
                    }
                }
            }

            _SEH2_TRY
            {
                LdrpFreeLoadContextOfNode(LdrEntry->DdagNode, OutStatus);

                if (!NT_SUCCESS(*OutStatus))
                {
                    *OutputLdrEntry = NULL;
                    LdrpDecrementNodeLoadCountEx(LdrEntry, FALSE);
                    LdrpDereferenceModule(LdrEntry);
                }
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                DPRINT1("FATAL: Exception 0x%08lX during load finalization for %wZ\n",
                    _SEH2_GetExceptionCode(), &LdrEntry->BaseDllName);
            }
            _SEH2_END;
        }
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
