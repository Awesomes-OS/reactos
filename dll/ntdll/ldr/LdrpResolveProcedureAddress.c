#include <ldrp.h>

NTSTATUS
NTAPI
LdrpResolveProcedureAddress(IN PLDR_DATA_TABLE_ENTRY LdrEntry,
                            IN PSTR Name,
                            IN ULONG Ordinal,
                            IN LDRP_RESOLVE_PROCEDURE_ADDRESS_FLAGS Flags,
                            OUT PVOID* ProcedureAddress)
{
    NTSTATUS Status;
    UINT32 AttemptCount = 0;
    LDRP_PATH_SEARCH_CONTEXT PathSearchContext = {0};
    PathSearchContext.OriginalFullDllName = LdrEntry->FullDllName;
    PLDR_DATA_TABLE_ENTRY CurrentLdrEntry = LdrEntry;

    while (TRUE)
    {
        Status = LdrpGetProcedureAddress(CurrentLdrEntry->DllBase, Name, Ordinal, ProcedureAddress);

        if (Status != STATUS_RETRY)
            break;

        if (Flags.NoForwarderResolution || AttemptCount++ >= 32)
        {
            Status = STATUS_INVALID_IMAGE_FORMAT;
            goto Quickie;
        }

        ANSI_STRING ForwardedDllName;

        Status = LdrpParseForwarderDescription(ProcedureAddress, &ForwardedDllName, &Name, &Ordinal);

        if (!NT_SUCCESS(Status))
            goto Quickie;

        PathSearchContext.SearchOptions = CurrentLdrEntry->ImplicitPathOptions;

        Status = LdrpLoadForwardedDll(&ForwardedDllName,
                                      &PathSearchContext,
                                      LdrEntry,
                                      CurrentLdrEntry,
                                      LoadReasonDynamicForwarderDependency,
                                      &CurrentLdrEntry);

        if (!NT_SUCCESS(Status))
            goto Quickie;

        LdrpDereferenceModule(CurrentLdrEntry);
    }

    if (NT_SUCCESS(Status) && Flags.DoSecurityVerification)
    {
        // todo: do Control Flow Guard security verification
    }

Quickie:

    if (!NT_SUCCESS(Status))
        *ProcedureAddress = NULL;

    return Status;
}
