#include <ldrp.h>

NTSTATUS
NTAPI
LdrpResolveForwarder(IN PVOID ForwarderPointer,
                     IN PLDR_DATA_TABLE_ENTRY ExportLdrEntry,
                     IN PLDR_DATA_TABLE_ENTRY ImportLdrEntry,
                     OUT PVOID* ProcedureAddress)
{
    NTSTATUS Status;
    UINT32 AttemptCount = 0;
    PLDR_DATA_TABLE_ENTRY CurrentLdrEntry = ExportLdrEntry;
    PLDRP_CSLIST_ENTRY LastDependencyEntry = NULL;

    DPRINT1("LDR: %s(%p, [%wZ], [%wZ], %p)\n",
            __FUNCTION__,
            ForwarderPointer,
            &ExportLdrEntry->BaseDllName,
            &ImportLdrEntry->BaseDllName,
            ProcedureAddress);

    while (TRUE)
    {
        ANSI_STRING ForwardedDllName;
        PSTR Name = NULL;
        ULONG Ordinal = 0;

        Status = LdrpParseForwarderDescription(ForwarderPointer, &ForwardedDllName, &Name, &Ordinal);

        if (!NT_SUCCESS(Status))
            goto Quickie;

        if (ForwardedDllName.Length != 5 || _strnicmp(ForwardedDllName.Buffer, "NTDLL", 5) != 0)
        {
            Status = LdrpLoadDependentModule(&ForwardedDllName,
                                             ImportLdrEntry->LoadContext,
                                             CurrentLdrEntry,
                                             LoadReasonStaticForwarderDependency,
                                             &CurrentLdrEntry,
                                             &LastDependencyEntry);

            if (!NT_SUCCESS(Status) || Status == STATUS_PENDING)
                goto Quickie;
        }
        else
        {
            CurrentLdrEntry = LdrpNtDllDataTableEntry;
        }

        Status = LdrpGetProcedureAddress(CurrentLdrEntry->DllBase,
                                         Name,
                                         Ordinal,
                                         ProcedureAddress);

        if (Status != STATUS_RETRY)
            break;

        if (AttemptCount++ >= 32)
        {
            Status = STATUS_INVALID_IMAGE_FORMAT;
            goto Quickie;
        }

        ForwarderPointer = *ProcedureAddress;
    }

    if (NT_SUCCESS(Status))
    {
        // todo: do Control Flow Guard security verification
    }

Quickie:

    if (LastDependencyEntry)
    {
        // We are left with an unused PLDRP_CSLIST_ENTRY allocated within LdrpLoadDependentModule
        LdrpHeapFree(0, LastDependencyEntry);
    }

    if (!NT_SUCCESS(Status))
        *ProcedureAddress = NULL;

    return Status;
}
