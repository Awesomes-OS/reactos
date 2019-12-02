#include <ldrp.h>
#include <reactos/ldrp.h>

NTSTATUS
NTAPI
LdrpMapDllWithSectionHandle(PLDRP_LOAD_CONTEXT LoadContext, HANDLE SectionHandle)
{
    PIMAGE_NT_HEADERS NtHeaders;
    SIZE_T ViewSize = 0;

    if (LoadContext->Entry->BaseDllName.Length <= 4 || !LoadContext->Entry->BaseDllName.Buffer || isspace(LoadContext->Entry->BaseDllName.Buffer[0]))
        __debugbreak();

    NTSTATUS Status = LdrpMinimalMapModule(LoadContext, SectionHandle, &ViewSize);

    if (!NT_SUCCESS(Status) || Status == STATUS_IMAGE_MACHINE_TYPE_MISMATCH)
    {
        return Status;
    }

    Status = RtlImageNtHeaderEx(0, LoadContext->Entry->DllBase, ViewSize, &NtHeaders);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    ULONG ComSectionSize;
    PIMAGE_COR20_HEADER ManagedDataImageDirectory;

    /* Setup the entry */
    if (LoadContext->Flags.Redirected) LoadContext->Entry->Redirected = TRUE;

    /* The image was valid. Is it a DLL? */
    if (NtHeaders->FileHeader.Characteristics & IMAGE_FILE_DLL)
    {
        /* Set the DLL Flag */
        LoadContext->Entry->ImageDll = TRUE;
    }

    /* If we're not a DLL, clear the entrypoint */
    if (!(LoadContext->Entry->ImageDll))
    {
        LoadContext->Entry->EntryPoint = NULL;
    }

    ManagedDataImageDirectory = (PIMAGE_COR20_HEADER)RtlImageDirectoryEntryToData(
        LoadContext->Entry->DllBase,
        TRUE,
        IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR,
        &ComSectionSize);

    if (ManagedDataImageDirectory)
    {
        LoadContext->Entry->CorImage = TRUE;
        if (ManagedDataImageDirectory->Flags & COMIMAGE_FLAGS_ILONLY)
            LoadContext->Entry->CorILOnly = TRUE;
    }

    PLDR_DATA_TABLE_ENTRY LoadedEntry = NULL;
    NTSTATUS FindStatus = LdrpFindLoadedDllByName(&LoadContext->Entry->BaseDllName,
                                                  LoadContext->Flags.HasFullPath
                                                      ? &LoadContext->Entry->FullDllName
                                                      : NULL,
                                                  LoadContext->Flags,
                                                  &LoadedEntry,
                                                  NULL);

    if (!NT_SUCCESS(FindStatus) || !LoadedEntry)
    {
        FindStatus = LdrpFindLoadedDllByMapping(LoadContext->Entry->DllBase, NtHeaders, &LoadedEntry, NULL);
    }

    if (NT_SUCCESS(FindStatus) && LoadedEntry)
    {
        // Our LDR_DATA_TABLE_ENTRY in LoadContext turned out to be temporary
        // since we already have this module loaded.
        // Replace with found entry.
        LdrpLoadContextReplaceModule(LoadContext, LoadedEntry);
        return Status;
    }

    RtlAcquireSRWLockExclusive(&LdrpModuleDatatableLock);

    LdrpInsertMemoryTableEntry(LoadContext->Entry);

    RtlReleaseSRWLockExclusive(&LdrpModuleDatatableLock);

    Status = LdrpCompleteMapModule(LoadContext, NtHeaders, Status, ViewSize);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = LdrpProcessMappedModule(LoadContext->Entry, LoadContext->Flags, TRUE);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    if (LoadContext->ParentEntry)
        LoadContext->Entry->ParentDllBase = LoadContext->ParentEntry->DllBase;

    if (LoadContext->Entry->ImageDll)
    {
        if (LoadContext->Entry->CorImage)
        {
            Status = LdrpCorProcessImports(LoadContext->Entry);
        }
        else
        {
            LdrpMapAndSnapDependency(LoadContext);
            Status = *LoadContext->StatusResponse;
        }
    }
    else
    {
        Status = STATUS_SUCCESS;
        LoadContext->Entry->DdagNode->State = LdrModulesReadyToRun;
    }

    return Status;
}
