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
    PLDR_DATA_TABLE_ENTRY LdrEntry = NULL;

    /* Sanity checks */
    ASSERT(OutLdrEntry);

    if (ContextFlags.BaseNameOnly)
    {
        Status = LdrpFindLoadedDllByName(BaseDllName, NULL, ContextFlags, &LdrEntry, NULL);
    }
    else if (ContextFlags.HasFullPath)
    {
        Status = LdrpFindLoadedDllByName(NULL, BaseDllName, ContextFlags, &LdrEntry, NULL);
    }

    if (Status == STATUS_DLL_NOT_FOUND)
    {
        Status = LdrpAllocatePlaceHolder(BaseDllName,
                                         PathSearchContext,
                                         ContextFlags,
                                         LoadReason,
                                         ParentEntry,
                                         &LdrEntry,
                                         OutStatus);
        if (!NT_SUCCESS(Status))
        {
            goto Quickie;
        }

        Status = LdrpLoadKnownDll(LdrEntry->LoadContext);
    }
    else if (LdrEntry && LdrEntry->DdagNode->State < 0)
    {
        if (ShowSnaps)
        {
            DPRINT1("LDR: %s: Found circular dependent DLL \"%wZ\" that failed to load previously, DDAG state: %d\n",
                    __FUNCTION__,
                    BaseDllName,
                    LdrEntry->DdagNode->State);
        }

        LdrpDereferenceModule(LdrEntry);
        Status = STATUS_INTERNAL_ERROR;
        LdrEntry = NULL;
    }
    else if (LdrEntry)
    {
        LdrpIncrementModuleLoadCount(LdrEntry);
    }

Quickie:
    *OutLdrEntry = LdrEntry;

    /* Return success */
    return Status;
}
