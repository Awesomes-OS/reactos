#include <ldrp.h>

#if 0
NTSTATUS
NTAPI
LdrpHandleOneOldFormatImportDescriptor(IN PLDRP_LOAD_CONTEXT LoadContext,
                                       IN PLDR_DATA_TABLE_ENTRY LdrEntry,
                                       IN PIMAGE_IMPORT_DESCRIPTOR ImportEntry)
{
    ANSI_STRING ImportName;
    NTSTATUS Status;
    PLDR_DATA_TABLE_ENTRY DllLdrEntry;
    PIMAGE_THUNK_DATA FirstThunk;

    /* Get the import name's VA */
    RtlInitAnsiString(&ImportName, PTR_ADD_OFFSET(LdrEntry->DllBase, ImportEntry->Name));

    /* Get the first thunk */
    FirstThunk = PTR_ADD_OFFSET(LdrEntry->DllBase, ImportEntry->FirstThunk);

    /* Make sure it's valid */
    if (!FirstThunk->u1.Function)
        goto SkipEntry;

    /* Show debug message */
    if (ShowSnaps)
    {
        DPRINT1("LDR: %Z used by %wZ\n",
                &ImportName,
                &LdrEntry->BaseDllName);
    }

    /* Load the module associated to it */
    Status = LdrpLoadDependentModule(&ImportName,
                                     LoadContext,
                                     LdrEntry,
                                     FALSE,
                                     &DllLdrEntry);

    LdrpAddDependency(LdrEntry, DllLdrEntry);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        if (ShowSnaps)
        {
            DPRINT1("LDR: LdrpWalkImportTable - LdrpLoadImportModule failed "
                     "on import %Z with status 0x%08lX\n",
                     &ImportName,
                     Status);
        }

        /* Return */
        return Status;
    }

    /* Show debug message */
    if (ShowSnaps)
    {
        DPRINT1("LDR: Snapping imports for %wZ from %Z\n",
                &LdrEntry->BaseDllName,
                &ImportName);
    }

    /* Now snap the IAT Entry */
    Status = LdrpSnapIAT(DllLdrEntry, LdrEntry, ImportEntry);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        if (ShowSnaps)
        {
            DPRINT1("LDR: LdrpWalkImportTable - LdrpSnapIAT #2 failed with "
                     "status 0x%08lX\n",
                     Status);
        }

        /* Return */
        return Status;
    }

SkipEntry:
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
LdrpHandleOldFormatImportDescriptors(IN PLDRP_LOAD_CONTEXT LoadContext,
                                     IN PLDR_DATA_TABLE_ENTRY LdrEntry,
                                     IN PIMAGE_IMPORT_DESCRIPTOR ImportEntry)
{
    NTSTATUS Status;

    /* Check for Name and Thunk */
    while ((ImportEntry->Name) && (ImportEntry->FirstThunk))
    {
        /* Parse this descriptor */
        Status = LdrpHandleOneOldFormatImportDescriptor(LoadContext,
                                                        LdrEntry,
                                                        ImportEntry);
        if (!NT_SUCCESS(Status))
            return Status;

        ImportEntry++;
    }

    /* Done */
    return STATUS_SUCCESS;
}

#endif