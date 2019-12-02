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

    LDR_DDAG_STATE DdagState = 0;

    Status = LdrpFindLoadedDllByName(BaseDllName,
                                     FullDllName,
                                     LoadContextFlags,
                                     OutputLdrEntry,
                                     &DdagState);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    if (DdagState != LdrModulesReadyToRun)
    {
        LdrpDereferenceModule(*OutputLdrEntry);
        *OutputLdrEntry = NULL;
        return Status;
    }

    Status = LdrpIncrementModuleLoadCount(*OutputLdrEntry);
    if (!NT_SUCCESS(Status))
    {
        LdrpDereferenceModule(*OutputLdrEntry);
        *OutputLdrEntry = NULL;
        return Status;
    }

    Status = LdrpBuildForwarderLink(ForwarderSource, *OutputLdrEntry);
    if (!NT_SUCCESS(Status))
    {
        LdrpDrainWorkQueue();
        LdrpDecrementNodeLoadCountEx(*OutputLdrEntry, FALSE);

        LdrpDereferenceModule(*OutputLdrEntry);
        *OutputLdrEntry = NULL;
        return Status;
    }

    return Status;
}