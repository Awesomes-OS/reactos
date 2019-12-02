#include <ldrp.h>


NTSTATUS
NTAPI
LdrpMapAndSnapDependency(PLDRP_LOAD_CONTEXT LoadContext)
{
    NTSTATUS Status = LdrpFindDllActivationContext(LoadContext->Entry);

    if (!NT_SUCCESS(Status))
        goto Quickie;

    Status = LdrpPrepareImportAddressTableForSnap(LoadContext);

    if (!NT_SUCCESS(Status))
        goto Quickie;

    if (!LoadContext->IATBaseAddress)
    {
        LoadContext->Entry->DdagNode->State = LdrModulesSnapped;
        goto Quickie;
    }

    LoadContext->IAT = LdrpGetImportDescriptorForSnap(LoadContext);

    if (LoadContext->IAT->Name)
    {
        ULONG CountOfDependencies = 0, CountOfValidDependencies = 0;

        for (PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor = LoadContext->IAT;
             ImportDescriptor->Name && ImportDescriptor->FirstThunk;
             ImportDescriptor++, CountOfDependencies++)
        {
            const PIMAGE_THUNK_DATA FirstThunk = PTR_ADD_OFFSET(LoadContext->Entry->DllBase,
                                                                ImportDescriptor->FirstThunk);

            if (FirstThunk->u1.Function)
                ++CountOfValidDependencies;
        }

        if (CountOfValidDependencies)
        {
            LoadContext->ImportEntries = LdrpHeapAlloc(HEAP_ZERO_MEMORY,
                                                       sizeof(PLDR_DATA_TABLE_ENTRY) * CountOfDependencies);

            if (!LoadContext->ImportEntries)
            {
                Status = STATUS_NO_MEMORY;
                goto Quickie;
            }

            LoadContext->ImportEntriesCount = CountOfDependencies;

            LoadContext->CountOfDependenciesPendingMap = CountOfValidDependencies + 1;

            ULONG Index = 0;
            PLDRP_CSLIST_ENTRY DependencyEntryPlaceholder = NULL;

            for (PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor = LoadContext->IAT;
                 ImportDescriptor->Name && ImportDescriptor->FirstThunk;
                 ImportDescriptor++, Index++)
            {
                const PIMAGE_THUNK_DATA FirstThunk = PTR_ADD_OFFSET(LoadContext->Entry->DllBase,
                                                                    ImportDescriptor->FirstThunk);
                if (FirstThunk->u1.Function)
                {
                    ANSI_STRING ImportName;

                    // Get the import name's VA
                    Status = RtlInitAnsiStringEx(&ImportName,
                                                 PTR_ADD_OFFSET(LoadContext->Entry->DllBase, ImportDescriptor->Name));

                    if (!NT_SUCCESS(Status))
                        break;

                    Status = LdrpLoadDependentModule(&ImportName,
                                                     LoadContext,
                                                     LoadContext->Entry,
                                                     LoadReasonStaticDependency,
                                                     &LoadContext->ImportEntries[Index],
                                                     &DependencyEntryPlaceholder);

                    if (!NT_SUCCESS(Status))
                        break;
                }
            }

            if (DependencyEntryPlaceholder)
            {
                // We are left with an unused PLDRP_CSLIST_ENTRY allocated within LdrpLoadDependentModule
                LdrpHeapFree(0, DependencyEntryPlaceholder);
            }

            if (NT_SUCCESS(Status))
            {
                RtlAcquireSRWLockExclusive(&LdrpModuleDatatableLock);
                CountOfValidDependencies = --LoadContext->CountOfDependenciesPendingMap;
                RtlReleaseSRWLockExclusive(&LdrpModuleDatatableLock);
            }

            if (CountOfValidDependencies)
            {
                goto Quickie;
            }
        }
    }

    LoadContext->Entry->DdagNode->State = LdrModulesSnapping;
    if (LoadContext->ParentEntry)
        LdrpQueueWork(LoadContext);
    else
        Status = LdrpSnapModule(LoadContext);

    // IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT (11) are not processed since Windows 8 Developer Preview

    // LdrpPrepareImportAddressTableForSnap: IMAGE_DIRECTORY_ENTRY_IAT (12)
    //                LdrImageDirectoryEntryToLoadConfig: IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG (10)
    // LdrpGetImportDescriptorForSnap: IMAGE_DIRECTORY_ENTRY_IMPORT (1)
    // LdrpSnapModule: IMAGE_DIRECTORY_ENTRY_EXPORT (0)

    // LdrpProcessMappedModule:
    //                LdrpCfgProcessLoadConfig: IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT (13)

Quickie:

    if (!NT_SUCCESS(Status))
    {
        *LoadContext->StatusResponse = Status;
    }

    return Status;
}
