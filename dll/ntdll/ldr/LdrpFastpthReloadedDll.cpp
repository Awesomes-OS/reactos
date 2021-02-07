#include <ldrp.h>

NTSTATUS
NTAPI
// ReSharper disable IdentifierTypo
LdrpFastpthReloadedDll(IN PUNICODE_STRING BaseDllName,
                       IN LDRP_LOAD_CONTEXT_FLAGS LoadContextFlags,
                       IN PLDR_DATA_TABLE_ENTRY ForwarderSource OPTIONAL,
                       OUT PLDR_DATA_TABLE_ENTRY* OutputLdrEntry)
{
    // ReSharper restore IdentifierTypo
    NTSTATUS Status = STATUS_NOT_FOUND;
    PUNICODE_STRING FullDllName;

    ASSERT(OutputLdrEntry);

    if (LoadContextFlags.BaseNameOnly)
    {
        FullDllName = NULL;
    }
    else if (LoadContextFlags.HasFullPath)
    {
        FullDllName = BaseDllName;
        BaseDllName = NULL;
    }
    else
    {
        return Status;
    }

    LDR_DDAG_STATE DdagState = LdrModulesPlaceHolder;

    Status = LdrpFindLoadedDllByName(BaseDllName,
                                     FullDllName,
                                     LoadContextFlags,
                                     OutputLdrEntry,
                                     &DdagState);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = STATUS_NOT_FOUND;

    if (DdagState == LdrModulesReadyToRun)
    {
        Status = LdrpIncrementModuleLoadCount(*OutputLdrEntry);
        if (NT_SUCCESS(Status))
        {
            Status = LdrpBuildForwarderLink(ForwarderSource, *OutputLdrEntry);
            if (!NT_SUCCESS(Status))
            {
                BOOLEAN LoadOwner = NtCurrentTeb()->LoadOwner;

                if (!LoadOwner)
                    LdrpDrainWorkQueue(LoadComplete);

                LdrpDecrementModuleLoadCountEx(*OutputLdrEntry, FALSE);

                if (!LoadOwner)
                    LdrpDropLastInProgressCount();
            }
        }
    }

    if (!NT_SUCCESS(Status))
    {
        LdrpDereferenceModule(*OutputLdrEntry);
        *OutputLdrEntry = NULL;
    }

    return Status;
}
