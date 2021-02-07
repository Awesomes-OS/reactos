#include <ldrp.h>
#include <reactos/ldrp.h>

NTSTATUS
NTAPI
LdrpMapDllWithSectionHandle(PLDRP_LOAD_CONTEXT LoadContext, HANDLE SectionHandle)
{
    PIMAGE_NT_HEADERS NtHeaders;
    SIZE_T ViewSize = 0;

    NTSTATUS Status = LdrpMinimalMapModule(LoadContext, SectionHandle, &ViewSize);

    if (!NT_SUCCESS(Status) || Status == STATUS_IMAGE_MACHINE_TYPE_MISMATCH)
    {
        return Status;
    }

    Status = RtlImageNtHeaderEx(0, LoadContext->Module->DllBase, ViewSize, &NtHeaders);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Setup the entry */
    if (LoadContext->Flags.Redirected)
        LoadContext->Module->Redirected = TRUE;

    PLDR_DATA_TABLE_ENTRY LoadedEntry = NULL;
    NTSTATUS FindStatus = LdrpFindLoadedDllByName(&LoadContext->Module->BaseDllName,
                                                  LoadContext->Flags.HasFullPath
                                                      ? &LoadContext->Module->FullDllName
                                                      : NULL,
                                                  LoadContext->Flags,
                                                  &LoadedEntry,
                                                  NULL);

    if (!NT_SUCCESS(FindStatus) || !LoadedEntry)
    {
        FindStatus = LdrpFindLoadedDllByMapping(LoadContext->Module->DllBase, NtHeaders, &LoadedEntry, NULL);
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

    LdrpInsertDataTableEntry(LoadContext->Module);
    LdrpInsertModuleToIndexLockHeld(LoadContext->Module, NtHeaders);

    RtlReleaseSRWLockExclusive(&LdrpModuleDatatableLock);

    Status = LdrpCompleteMapModule(LoadContext, NtHeaders, Status, ViewSize);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = LdrpProcessMappedModule(LoadContext->Module, LoadContext->Flags, TRUE);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    if (LoadContext->ParentModule)
        LoadContext->Module->ParentDllBase = LoadContext->ParentModule->DllBase;

    if (LoadContext->Module->ImageDll)
    {
        if (LoadContext->Module->CorImage)
        {
            Status = LdrpCorProcessImports(LoadContext->Module);
        }
        else
        {
            LdrpMapAndSnapDependency(LoadContext);
            Status = *LoadContext->LoadStatus;
        }
    }
    else
    {
        ASSERT(!LoadContext->Module->EntryPoint);
        ASSERT(!LoadContext->Module->TlsIndex);

        Status = STATUS_SUCCESS;
        LoadContext->Module->DdagNode->State = LdrModulesReadyToRun;
    }

    return Status;
}
