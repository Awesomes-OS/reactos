#include <ldrp.h>

NTSTATUS
NTAPI
LdrpSnapThunk(IN PIMAGE_EXPORT_DIRECTORY ExportDir,
              IN ULONG ExportDirSize,
              IN PVOID ExportDirPointer,
              IN PLDR_DATA_TABLE_ENTRY ExportEntry,
              IN PLDR_DATA_TABLE_ENTRY ImportEntry,
              IN PIMAGE_THUNK_DATA ExportThunk,
              OUT PVOID* ImportThunk)
{
    NTSTATUS Status;
    ULONG ForwarderOrdinal = 0;
    PSTR ForwarderName = NULL;

    const BOOLEAN SnapByOrdinal = IMAGE_SNAP_BY_ORDINAL(ExportThunk->u1.Ordinal);

    /* Check if the snap is by ordinal */
    if (SnapByOrdinal)
    {
        /* Get the ordinal number, and its normalized version */
        ForwarderOrdinal = IMAGE_ORDINAL(ExportThunk->u1.Ordinal);
    }
    else
    {
        /* First get the data VA */
        const auto* const AddressOfData = (PIMAGE_IMPORT_BY_NAME)PTR_ADD_OFFSET(ImportEntry->DllBase,
                                                                   ExportThunk->u1.AddressOfData);

        ForwarderName = (LPSTR)AddressOfData->Name;
    }

    Status = LdrpGetProcedureAddressImpl(ExportDir,
                                         ExportDirSize,
                                         ExportDirPointer,
                                         ExportEntry->DllBase,
                                         ForwarderName,
                                         ForwarderOrdinal,
                                         ImportThunk);

    /* Check if the ordinal is invalid */
    if (Status == STATUS_ENTRYPOINT_NOT_FOUND || Status == STATUS_ORDINAL_NOT_FOUND)
    {
        Status = STATUS_PROCEDURE_NOT_FOUND;
    }
    else if (Status == STATUS_RETRY)
    {
        Status = LdrpResolveForwarder((PVOID)*ImportThunk, ExportEntry, ImportEntry, ImportThunk);
    }

    if (Status == STATUS_PROCEDURE_NOT_FOUND || Status == STATUS_DLL_NOT_FOUND)
    {
        // Recover original status (what were we snapping in the first place?)
        if (SnapByOrdinal)
            Status = STATUS_ORDINAL_NOT_FOUND;
        else
            Status = STATUS_ENTRYPOINT_NOT_FOUND;

        ANSI_STRING ForwarderNameStorage = {0};
        RtlInitAnsiString(&ForwarderNameStorage, ForwarderName);

        LdrpReportSnapError(ImportEntry->DllBase,
                            &ImportEntry->BaseDllName,
                            &ForwarderNameStorage,
                            ForwarderOrdinal,
                            Status,
                            TRUE);
    }

    /* Return status */
    return Status;
}


NTSTATUS
NTAPI
LdrpSnapIAT(IN PLDR_DATA_TABLE_ENTRY ExportLdrEntry,
            IN PLDRP_LOAD_CONTEXT LoadContext)
{
    NTSTATUS Status;

    DPRINT("LDR: %s(\"%wZ\", \"%wZ\")\n",
           __FUNCTION__,
           &ExportLdrEntry->BaseDllName,
           &LoadContext->DllName);

    const PIMAGE_IMPORT_DESCRIPTOR IatEntry = LoadContext->ImportDescriptors +
        LoadContext->ImportIndex;

    PIMAGE_EXPORT_DIRECTORY ExportDir = NULL;
    ULONG ExportDirSize;
    PVOID ExportDirPointer = NULL;

    /* Get the pointer to the export directory */
    Status = RtlImageDirectoryEntryToDataEx(ExportLdrEntry->DllBase,
                                            TRUE,
                                            IMAGE_DIRECTORY_ENTRY_EXPORT,
                                            &ExportDirSize,
                                            &ExportDirPointer);
    if (NT_SUCCESS(Status))
        ExportDir = static_cast<PIMAGE_EXPORT_DIRECTORY>(ExportDirPointer);

    if (!ExportDir)
    {
        DPRINT1("LDR: %s(\"%wZ\", ...): No export table\n",
                __FUNCTION__,
                &ExportLdrEntry->BaseDllName);

        if (LdrpDebugFlags.BreakInDebugger)
            __debugbreak();

        return STATUS_INVALID_IMAGE_FORMAT;
    }

    ASSERT(IatEntry->FirstThunk);

    const PLDR_DATA_TABLE_ENTRY ImportLdrEntry = LoadContext->Module;
    const PVOID ImportBaseAddress = ImportLdrEntry->DllBase;

    auto* const FirstImportThunk = (PIMAGE_THUNK_DATA)PTR_ADD_OFFSET(ImportBaseAddress, IatEntry->FirstThunk);

    PIMAGE_THUNK_DATA OriginalExportThunk;

    /* Get the Original thunk VA, watch out for weird images */
    if (!IatEntry->OriginalFirstThunk ||
        (IatEntry->OriginalFirstThunk > ImportLdrEntry->SizeOfImage))
    {
        /* Refuse it, this is a strange linked file */
        OriginalExportThunk = FirstImportThunk;
    }
    else
    {
        /* Get the address from the field and convert to VA */
        OriginalExportThunk = (PIMAGE_THUNK_DATA)PTR_ADD_OFFSET(ImportBaseAddress, IatEntry->OriginalFirstThunk);
    }

    PIMAGE_THUNK_DATA CurrentImportThunk = FirstImportThunk + LoadContext->ThunkIndex;
    PIMAGE_THUNK_DATA CurrentExportThunk = OriginalExportThunk + LoadContext->ThunkIndex;

    /* Loop while it's valid */
    while (CurrentExportThunk->u1.AddressOfData)
    {
        UINT_PTR ImportThunkValue = ~0xFFFFFF | 0xBADD11;

        /* Snap the Thunk */
        Status = LdrpSnapThunk(ExportDir,
                               ExportDirSize,
                               ExportDirPointer,
                               ExportLdrEntry,
                               ImportLdrEntry,
                               CurrentExportThunk,
                               (PVOID*)&ImportThunkValue);

        // If we stumbled upon forwarder, that is not yet resolved, break out
        if (Status == STATUS_PENDING)
            return Status;

        /* If we failed the snap, break out */
        if (!NT_SUCCESS(Status))
            break;

        // Save the snapped function pointer
        CurrentImportThunk->u1.Function = ImportThunkValue;

        // Move to the next thunks
        ++LoadContext->ThunkIndex;
        CurrentImportThunk = FirstImportThunk + LoadContext->ThunkIndex;
        CurrentExportThunk = OriginalExportThunk + LoadContext->ThunkIndex;
    }

    if (NT_SUCCESS(Status))
    {
        LoadContext->ThunkIndex = 0;
    }

    return Status;
}

NTSTATUS
NTAPI
LdrpSnapModule(IN PLDRP_LOAD_CONTEXT LoadContext)
{
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT1("LDR: %s(" LDR_HEXPTR_FMT "[\"%wZ\"])\n", __FUNCTION__, LoadContext, &LoadContext->DllName);

    ASSERT(LoadContext->Module->DdagNode->State == LdrModulesSnapping);
    ASSERT(!LoadContext->UnmappedChildCount);
    ASSERT(LoadContext->ImportIndex < LoadContext->ImportCount);

    LdrpHandlePendingModuleReplaced(LoadContext);

    for (;
        LoadContext->ImportIndex < LoadContext->ImportCount;
        ++LoadContext->ImportIndex)
    {
        const PLDR_DATA_TABLE_ENTRY ImportEntry = LoadContext->ImportArray[LoadContext->ImportIndex];

        const PLDR_DATA_TABLE_ENTRY NewEntry = LdrpHandleReplacedModule(ImportEntry);

        if (NewEntry != ImportEntry)
        {
            LdrpFreeReplacedModule(ImportEntry);
            LoadContext->ImportArray[LoadContext->ImportIndex] = NewEntry;
        }

        if (NewEntry)
        {
            Status = LdrpSnapIAT(NewEntry, LoadContext);

            if (Status == STATUS_PENDING)
                goto Quickie;

            if (!NT_SUCCESS(Status))
                break;
        }
    }

    if (NT_SUCCESS(Status))
    {
        Status = LdrpDoPostSnapWork(LoadContext);
        if (NT_SUCCESS(Status))
        {
            LoadContext->Module->DdagNode->State = LdrModulesSnapped;
        }
    }

    // todo: Unmap LDRP_LOAD_CONTEXT->ProcessImageSectionViewBase (LdrpGetImportDescriptorForSnap)

Quickie:

    DPRINT1("LDR: %s(" LDR_HEXPTR_FMT "[\"%wZ\"]): Status: 0x%08lX\n", __FUNCTION__, LoadContext, &LoadContext->DllName, Status);

    if (Status == STATUS_PENDING)
        Status = STATUS_SUCCESS;

    /* Return status */
    return Status;
}
