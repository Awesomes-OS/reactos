#include <ldrp.h>
#include <ntintsafe.h>

NTSTATUS
NTAPI
LdrpGetProcedureAddressImpl(IN PIMAGE_EXPORT_DIRECTORY ExportDir,
                            IN ULONG ExportDirSize,
                            IN PVOID ExportDirPointer,
                            IN PVOID BaseAddress,
                            IN PSTR Name,
                            IN ULONG Ordinal,
                            OUT PVOID* ProcedureAddress)
{
    if (Name)
    {
#if 0
        if (LdrpDebugFlags.LogTrace)
            DPRINT1("LDR: Locating procedure \"%s\" by name\n", Name);
#endif

        const UINT32 OrdinalResult = LdrpNameToOrdinal(Name,
                                                       ExportDir->NumberOfNames,
                                                       BaseAddress,
                                                       (PULONG)PTR_ADD_OFFSET(BaseAddress, ExportDir->AddressOfNames),
                                                       (PUSHORT)PTR_ADD_OFFSET(BaseAddress, ExportDir->AddressOfNameOrdinals));

        if (OrdinalResult == UINT32_MAX)
            return STATUS_PROCEDURE_NOT_FOUND;

        Ordinal = OrdinalResult;
    }
    else
    {
        if (LdrpDebugFlags.LogTrace)
            DPRINT1("LDR: Locating procedure 0x%lx by ordinal\n", Ordinal);

        if (!Ordinal)
            return STATUS_INVALID_PARAMETER;

        Ordinal -= ExportDir->Base;
    }

    if (Ordinal >= ExportDir->NumberOfFunctions)
    {
        if (Name)
            return STATUS_ENTRYPOINT_NOT_FOUND;
        else
            return STATUS_ORDINAL_NOT_FOUND;
    }

    const auto* FunctionTable = (PULONG32)PTR_ADD_OFFSET(BaseAddress, ExportDir->AddressOfFunctions);

    const PVOID Function = PTR_ADD_OFFSET(BaseAddress, FunctionTable[Ordinal]);

    *ProcedureAddress = Function;

    // Check if we have already snapped the function
    // If we have, Function pointer will be outside the IMAGE_EXPORT_DIRECTORY
    NTSTATUS Status = STATUS_RETRY;

    if (Function < ExportDirPointer)
        Status = STATUS_SUCCESS;
    else if (Function >= PTR_ADD_OFFSET(ExportDirPointer, ExportDirSize))
        Status = STATUS_SUCCESS;

    /* We're done */
    return Status;
}

EXTERN_C
NTSTATUS
NTAPI
LdrpGetProcedureAddress(IN PVOID BaseAddress,
                        IN PSTR Name,
                        IN ULONG Ordinal,
                        OUT PVOID *ProcedureAddress)
{
    NTSTATUS Status;
    PIMAGE_EXPORT_DIRECTORY ExportDir = NULL;
    ULONG ExportDirSize;
    PVOID ExportDirPointer = NULL;

    /* Get the pointer to the export directory */
    Status = RtlImageDirectoryEntryToDataEx(BaseAddress,
                                            TRUE,
                                            IMAGE_DIRECTORY_ENTRY_EXPORT,
                                            &ExportDirSize,
                                            &ExportDirPointer);
    if (NT_SUCCESS(Status))
        ExportDir = static_cast<PIMAGE_EXPORT_DIRECTORY>(ExportDirPointer);

    if (!ExportDir)
    {
        DPRINT1("LDR: %s(" LDR_HEXPTR_FMT ", \"%s\", %lu, ...): No exports\n",
                __FUNCTION__,
                BaseAddress,
                Name,
                Ordinal);

        return STATUS_PROCEDURE_NOT_FOUND;
    }

    return LdrpGetProcedureAddressImpl(ExportDir, ExportDirSize, ExportDirPointer,
                                       BaseAddress, Name, Ordinal, ProcedureAddress);
}
