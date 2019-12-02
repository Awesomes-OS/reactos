#include <ldrp.h>

#if 0
NTSTATUS
NTAPI
LdrpHandleOneNewFormatImportDescriptor(IN PLDRP_LOAD_CONTEXT LoadContext,
                                       IN PLDR_DATA_TABLE_ENTRY LdrEntry,
                                       IN PIMAGE_BOUND_IMPORT_DESCRIPTOR *BoundEntryPtr,
                                       IN PIMAGE_BOUND_IMPORT_DESCRIPTOR FirstEntry)
{
    ANSI_STRING BoundImportName;
    NTSTATUS Status;
    BOOLEAN Stale;
    PIMAGE_IMPORT_DESCRIPTOR ImportEntry;
    PLDR_DATA_TABLE_ENTRY DllLdrEntry, ForwarderLdrEntry;
    PIMAGE_BOUND_FORWARDER_REF ForwarderEntry;
    PIMAGE_BOUND_IMPORT_DESCRIPTOR BoundEntry;
    ULONG i, IatSize;

    /* Get the pointer to the bound entry */
    BoundEntry = *BoundEntryPtr;

    /* Get the name's VA */
    RtlInitAnsiString(&BoundImportName, PTR_ADD_OFFSET(FirstEntry, BoundEntry->OffsetModuleName));

    /* Show debug message */
    if (ShowSnaps)
    {
        DPRINT1("LDR: %wZ bound to %Z\n", &LdrEntry->BaseDllName, &BoundImportName);
    }

    /* Load the module for this entry */
    Status = LdrpLoadDependentModule(&BoundImportName,
                                     LoadContext,
                                     LdrEntry,
                                     FALSE,
                                     &DllLdrEntry);
    LdrpAddDependency(LdrEntry, DllLdrEntry);
    if (!NT_SUCCESS(Status))
    {
        /* Show debug message */
        if (ShowSnaps)
        {
            DPRINT1("LDR: %wZ failed to load import module %Z; 0x%08lX\n",
                    &LdrEntry->BaseDllName,
                    &BoundImportName,
                    Status);
        }
        goto Quickie;
    }

    /* Check if the Bound Entry is now invalid */
    if (BoundEntry->TimeDateStamp != DllLdrEntry->TimeDateStamp)
    {
        /* Show debug message */
        if (ShowSnaps)
        {
            DPRINT1("LDR: %wZ has stale binding to %Z\n",
                    &LdrEntry->BaseDllName,
                    &BoundImportName);
        }

        /* Remember it's become stale */
        Stale = TRUE;
    }
    else
    {
        /* Show debug message */
        if (ShowSnaps)
        {
            DPRINT1("LDR: %wZ has correct binding to %Z\n",
                    &LdrEntry->BaseDllName,
                    &BoundImportName);
        }

        /* Remember it's valid */
        Stale = FALSE;
    }

    /* Get the forwarders */
    ForwarderEntry = (PIMAGE_BOUND_FORWARDER_REF) (BoundEntry + 1);

    /* Loop them */
    for (i = 0; i < BoundEntry->NumberOfModuleForwarderRefs; i++)
    {
        ANSI_STRING ForwarderName;
        /* Get the name */
        RtlInitAnsiString(&ForwarderName, PTR_ADD_OFFSET(FirstEntry, ForwarderEntry->OffsetModuleName));

        /* Show debug message */
        if (ShowSnaps)
        {
            DPRINT1("LDR: %wZ bound to %Z via forwarder(s) from %wZ\n",
                    &LdrEntry->BaseDllName,
                    &ForwarderName,
                    &DllLdrEntry->BaseDllName);
        }

        /* Load the module */
        Status = LdrpLoadDependentModule(&ForwarderName,
                                         LoadContext,
                                         DllLdrEntry,
                                         TRUE,
                                         &ForwarderLdrEntry);

        LdrpAddDependency(DllLdrEntry, ForwarderLdrEntry);

        /* Check if the Bound Entry is now invalid */
        if (!(NT_SUCCESS(Status)) ||
            (ForwarderEntry->TimeDateStamp != ForwarderLdrEntry->TimeDateStamp))
        {
            /* Show debug message */
            if (ShowSnaps)
            {
                DPRINT1("LDR: %wZ has stale binding to %Z\n",
                        &LdrEntry->BaseDllName,
                        &ForwarderName);
            }

            /* Remember it's become stale */
            Stale = TRUE;
        }
        else
        {
            /* Show debug message */
            if (ShowSnaps)
            {
                DPRINT1("LDR: %wZ has correct binding to %Z\n",
                        &LdrEntry->BaseDllName,
                        &ForwarderName);
            }

            /* Remember it's valid */
            Stale = FALSE;
        }

        /* Move to the next one */
        ForwarderEntry++;
    }

    /* Set the next bound entry to the forwarder */
    FirstEntry = (PIMAGE_BOUND_IMPORT_DESCRIPTOR) ForwarderEntry;

    /* Check if the binding was stale */
    if (Stale)
    {
        /* It was, so find the IAT entry for it */
        ImportEntry = RtlImageDirectoryEntryToData(LdrEntry->DllBase,
                                                   TRUE,
                                                   IMAGE_DIRECTORY_ENTRY_IMPORT,
                                                   &IatSize);

        /* Make sure it has a name */
        while (ImportEntry->Name)
        {
            /* Get the name */
            LPSTR ImportName = PTR_ADD_OFFSET(LdrEntry->DllBase, ImportEntry->Name);

            /* Compare it */
            if (!_stricmp(ImportName, BoundImportName.Buffer))
                break;

            /* Move to next entry */
            ImportEntry++;
        }

        /* If we didn't find a name, fail */
        if (!ImportEntry->Name)
        {
            /* Show debug message */
            if (ShowSnaps)
            {
                DPRINT1("LDR: LdrpWalkImportTable - failing with"
                        "STATUS_OBJECT_NAME_INVALID due to no import descriptor name\n");
            }

            /* Return error */
            Status = STATUS_OBJECT_NAME_INVALID;
            goto Quickie;
        }

        /* Show debug message */
        if (ShowSnaps)
        {
            LPSTR ImportName = PTR_ADD_OFFSET(LdrEntry->DllBase, ImportEntry->Name);

            DPRINT1("LDR: Stale Bind %s from %wZ\n",
                    ImportName,
                    &LdrEntry->BaseDllName);
        }

        /* Snap the IAT Entry */
        Status = LdrpSnapIAT(DllLdrEntry,
                             LdrEntry,
                             ImportEntry,
                             FALSE);

        /* Make sure we didn't fail */
        if (!NT_SUCCESS(Status))
        {
            /* Show debug message */
            if (ShowSnaps)
            {
                DPRINT1("LDR: %wZ failed to load import module %s; status = %x\n",
                        &LdrEntry->BaseDllName,
                        BoundImportName,
                        Status);
            }

            /* Return */
            goto Quickie;
        }
    }

    /* All done */
    Status = STATUS_SUCCESS;

Quickie:
    /* Write where we are now and return */
    *BoundEntryPtr = FirstEntry;
    return Status;
}

NTSTATUS
NTAPI
LdrpHandleNewFormatImportDescriptors(IN PLDRP_LOAD_CONTEXT LoadContext,
                                     IN PLDR_DATA_TABLE_ENTRY LdrEntry,
                                     IN PIMAGE_BOUND_IMPORT_DESCRIPTOR BoundEntry)
{
    PIMAGE_BOUND_IMPORT_DESCRIPTOR FirstEntry = BoundEntry;
    NTSTATUS Status;

    /* Make sure we have a name */
    while (BoundEntry->OffsetModuleName)
    {
        /* Parse this descriptor */
        Status = LdrpHandleOneNewFormatImportDescriptor(LoadContext,
                                                        LdrEntry,
                                                        &BoundEntry,
                                                        FirstEntry);
        if (!NT_SUCCESS(Status))
            return Status;
    }

    /* Done */
    return STATUS_SUCCESS;
}
#endif