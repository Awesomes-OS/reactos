#include <ldrp.h>


NTSTATUS
NTAPI
LdrpMapAndSnapDependency(PLDRP_LOAD_CONTEXT LoadContext)
{
    ASSERT(!LoadContext->ImportCount);
    ASSERT(!LoadContext->UnmappedChildCount);
    ASSERT(!LoadContext->ImportDescriptors);
    ASSERT(!LoadContext->ImportArray);

    NTSTATUS Status = LdrpFindDllActivationContext(LoadContext->Module);

    if (!NT_SUCCESS(Status))
        goto Quickie;

    Status = LdrpPrepareImportAddressTableForSnap(LoadContext);

    if (!NT_SUCCESS(Status))
        goto Quickie;

    if (!LoadContext->IATBase)
    {
        LoadContext->Module->DdagNode->State = LdrModulesSnapped;
        goto Quickie;
    }

    LoadContext->ImportDescriptors = LdrpGetImportDescriptorForSnap(LoadContext);

    if (LoadContext->ImportDescriptors->Name)
    {
        ULONG CountOfDependencies = 0, CountOfValidDependencies = 0;

        for (PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor = LoadContext->ImportDescriptors;
             ImportDescriptor->Name && ImportDescriptor->FirstThunk;
             ImportDescriptor++, CountOfDependencies++)
        {
            const auto* const FirstThunk = (PIMAGE_THUNK_DATA)PTR_ADD_OFFSET(LoadContext->Module->DllBase,
                                                                ImportDescriptor->FirstThunk);

            if (FirstThunk->u1.Function)
                ++CountOfValidDependencies;
        }

        if (CountOfValidDependencies)
        {
            LoadContext->ImportArray = new PLDR_DATA_TABLE_ENTRY[CountOfDependencies];

            if (!LoadContext->ImportArray)
            {
                Status = STATUS_NO_MEMORY;
                goto Quickie;
            }

            LoadContext->ImportCount = CountOfDependencies;

            LoadContext->UnmappedChildCount = CountOfValidDependencies + 1;

            ULONG Index = 0;
            PLDRP_CSLIST_ENTRY DependencyEntryPlaceholder = NULL;

            for (PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor = LoadContext->ImportDescriptors;
                 ImportDescriptor->Name && ImportDescriptor->FirstThunk;
                 ImportDescriptor++, Index++)
            {
                const auto* const FirstThunk = (PIMAGE_THUNK_DATA)PTR_ADD_OFFSET(LoadContext->Module->DllBase,
                                                                    ImportDescriptor->FirstThunk);

                if (!FirstThunk->u1.Function)
                    continue;

                ANSI_STRING ImportName;

                // Get the import name's VA
                Status = RtlInitAnsiStringEx(&ImportName,
                                             (PCSZ)PTR_ADD_OFFSET(LoadContext->Module->DllBase, ImportDescriptor->Name));

                if (!NT_SUCCESS(Status))
                    break;

                Status = LdrpLoadDependentModule(&ImportName,
                                                 LoadContext,
                                                 LoadContext->Module,
                                                 LoadReasonStaticDependency,
                                                 &LoadContext->ImportArray[Index],
                                                 &DependencyEntryPlaceholder);

                LDRP_ASSERT_CSLIST_ENTRY_OPTIONAL(DependencyEntryPlaceholder);

                if (!NT_SUCCESS(Status))
                    break;
            }

            if (DependencyEntryPlaceholder)
            {
                // We are left with an unused PLDRP_CSLIST_ENTRY allocated within LdrpLoadDependentModule
                LdrpHeapFree(0, DependencyEntryPlaceholder);
            }

            if (NT_SUCCESS(Status))
            {
                RtlAcquireSRWLockExclusive(&LdrpModuleDatatableLock);
                CountOfValidDependencies = --LoadContext->UnmappedChildCount;
                RtlReleaseSRWLockExclusive(&LdrpModuleDatatableLock);
            }

            if (CountOfValidDependencies)
            {
                goto Quickie;
            }
        }
    }

    LoadContext->Module->DdagNode->State = LdrModulesSnapping;
    if (LoadContext->ParentModule)
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
        *LoadContext->LoadStatus = Status;
    }

    return Status;
}
