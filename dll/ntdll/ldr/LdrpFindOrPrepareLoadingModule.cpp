#include <ldrp.h>
#include <reactos/ldrp.h>

NTSTATUS
NTAPI
LdrpFindOrPrepareLoadingModule(IN PUNICODE_STRING BaseDllName,
                               IN PLDRP_PATH_SEARCH_CONTEXT PathSearchContext,
                               IN LDRP_LOAD_CONTEXT_FLAGS ContextFlags,
                               IN LDR_DLL_LOAD_REASON LoadReason,
                               IN PLDR_DATA_TABLE_ENTRY ParentEntry,
                               OUT PLDR_DATA_TABLE_ENTRY* OutLdrEntry,
                               OUT NTSTATUS* OutStatus)
{
    NTSTATUS Status = STATUS_DLL_NOT_FOUND;
    PLDR_DATA_TABLE_ENTRY Module = NULL;

    /* Sanity checks */
    ASSERT(OutLdrEntry);

    if (ContextFlags.BaseNameOnly)
    {
        Status = LdrpFindLoadedDllByName(BaseDllName, NULL, ContextFlags, &Module, NULL);
    }
    else if (ContextFlags.HasFullPath)
    {
        Status = LdrpFindLoadedDllByName(NULL, BaseDllName, ContextFlags, &Module, NULL);
    }

    if (Status == STATUS_DLL_NOT_FOUND)
    {
        Status = LdrpAllocatePlaceHolder(BaseDllName,
                                         PathSearchContext,
                                         ContextFlags,
                                         LoadReason,
                                         ParentEntry,
                                         &Module,
                                         OutStatus);
        if (!NT_SUCCESS(Status))
        {
            goto Quickie;
        }

        Status = LdrpLoadKnownDll(Module->LoadContext);
    }
    else if (Module && Module->DdagNode->State < LdrModulesPlaceHolder)
    {
        if (LdrpDebugFlags.LogInformation)
        {
            DPRINT1("LDR: %s: Found circular dependent DLL \"%wZ\" that failed to load previously, DDAG state: %d\n",
                    __FUNCTION__,
                    BaseDllName,
                    Module->DdagNode->State);
        }

        LdrpDereferenceModule(Module);
        Status = STATUS_INTERNAL_ERROR;
        Module = NULL;
    }
    else if (Module)
    {
        LdrpIncrementModuleLoadCount(Module);
    }

Quickie:
    *OutLdrEntry = Module;

    /* Return success */
    return Status;
}
