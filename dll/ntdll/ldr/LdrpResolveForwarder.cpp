#include <ldrp.h>

NTSTATUS
NTAPI
LdrpResolveForwarder(IN PVOID ForwarderPointer,
                     IN PLDR_DATA_TABLE_ENTRY ExportModule,
                     IN PLDR_DATA_TABLE_ENTRY ImportModule,
                     OUT PVOID* ProcedureAddress)
{
    ASSERT(ForwarderPointer);
    LDRP_ASSERT_MODULE_ENTRY(ExportModule);
    LDRP_ASSERT_MODULE_ENTRY(ImportModule);
    ASSERT(ProcedureAddress);

    const PLDRP_LOAD_CONTEXT ImportContext = ImportModule->LoadContext;

    LDRP_ASSERT_LOAD_CONTEXT(ImportContext);
    ASSERT(!ImportContext->UnmappedChildCount);
    ASSERT(!ImportContext->PendingModule);

    NTSTATUS Status;
    UINT32 AttemptCount = 0;
    PLDR_DATA_TABLE_ENTRY CurrentModule = ExportModule;
    PLDRP_CSLIST_ENTRY LastDependencyEntry = NULL;

    DPRINT1("LDR: %s(" LDR_HEXPTR_FMT ", [%wZ], [%wZ], " LDR_HEXPTR_FMT ")\n",
            __FUNCTION__,
            ForwarderPointer,
            &ExportModule->BaseDllName,
            &ImportModule->BaseDllName,
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
                                             ImportContext,
                                             CurrentModule,
                                             LoadReasonStaticForwarderDependency,
                                             &CurrentModule,
                                             &LastDependencyEntry);

            LDRP_ASSERT_CSLIST_ENTRY_OPTIONAL(LastDependencyEntry);

            if (!NT_SUCCESS(Status) || Status == STATUS_PENDING)
                goto Quickie;
        }
        else
        {
            CurrentModule = LdrpNtDllDataTableEntry;

        }

        Status = LdrpGetProcedureAddress(CurrentModule->DllBase,
                                         Name,
                                         Ordinal,
                                         ProcedureAddress);

        if (Status != STATUS_RETRY)
            break;

        ASSERT(CurrentModule != LdrpNtDllDataTableEntry);

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
        // RtlGuardCheckImageBase
    }

Quickie:

    LDRP_ASSERT_CSLIST_ENTRY_OPTIONAL(LastDependencyEntry);

    if (LastDependencyEntry)
    {
        // We are left with an unused PLDRP_CSLIST_ENTRY allocated within LdrpLoadDependentModule
        LdrpHeapFree(0, LastDependencyEntry);
    }

    if (!NT_SUCCESS(Status))
        *ProcedureAddress = NULL;

    return Status;
}
