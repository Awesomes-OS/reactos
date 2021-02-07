#include <ldrp.h>

EXTERN_C
{

#include <ndk/exfuncs.h>
#include <ndk/mmfuncs.h>

}

NTSTATUS
NTAPI
LdrpCompleteMapModule(PLDRP_LOAD_CONTEXT LoadContext, PIMAGE_NT_HEADERS NtHeaders, NTSTATUS ImageStatus, SIZE_T ViewSize)
{
    PPEB Peb = NtCurrentPeb();
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Cor20HeaderSize;

    PIMAGE_COR20_HEADER Cor20Header = (PIMAGE_COR20_HEADER)RtlImageDirectoryEntryToData(
        LoadContext->Module->DllBase,
        TRUE,
        IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR,
        &Cor20HeaderSize);

    if (Cor20Header)
    {
        LoadContext->Module->CorImage = TRUE;
        if (Cor20Header->Flags & COMIMAGE_FLAGS_ILONLY)
        {
            LoadContext->Module->CorILOnly = TRUE;
            Status = LdrpCorValidateImage(LoadContext->Module->DllBase, NULL);
#if defined(_M_AMD64) || defined(_WIN64)
            if (NT_SUCCESS(Status) && LoadContext->Flags.CorImage)
            {
                Status = LdrpCorFixupImage(LoadContext->Entry->DllBase);
            }
#endif
        }
    }

    if (NT_SUCCESS(Status))
    {
        /* Set the DLL Flag */
        LoadContext->Module->ImageDll = NtHeaders->FileHeader.Characteristics & IMAGE_FILE_DLL ? TRUE : FALSE;
    }

    /* Check if we loaded somewhere else */
    if (NT_SUCCESS(Status) && ImageStatus == STATUS_IMAGE_NOT_AT_BASE && !LoadContext->Module->CorILOnly)
    {
        BOOLEAN OverlapDllFound = FALSE;
        UNICODE_STRING OverlapDll;
        BOOLEAN RelocatableDll = TRUE;

        ULONG_PTR ImageBase, ImageEnd;
        PLIST_ENTRY ListHead, NextEntry;

        /* Are we dealing with a DLL? */
        BOOL ApplyRelocs = LoadContext->Module->ImageDll;

        /* Find our region */
        ImageBase = (ULONG_PTR)NtHeaders->OptionalHeader.ImageBase;
        ImageEnd = ImageBase + ViewSize;

        DPRINT1("LDR: LdrpMapDll Relocating Image Name %wZ (" LDR_HEXPTR_FMT "-" LDR_HEXPTR_FMT " -> " LDR_HEXPTR_FMT ")\n",
                LoadContext->Module->BaseDllName, (PVOID)ImageBase, (PVOID)ImageEnd, LoadContext->Module->DllBase);

        /* Scan all the modules */
        ListHead = &Peb->Ldr->InLoadOrderModuleList;
        for (NextEntry = ListHead->Flink; NextEntry != ListHead; NextEntry = NextEntry->Flink)
        {
            PLDR_DATA_TABLE_ENTRY CandidateEntry;
            ULONG_PTR CandidateBase, CandidateEnd;

            /* Get the entry */
            CandidateEntry = CONTAINING_RECORD(NextEntry,
                                               LDR_DATA_TABLE_ENTRY,
                                               InLoadOrderLinks);

            /* Get the entry's bounds */
            CandidateBase = (ULONG_PTR)CandidateEntry->DllBase;
            CandidateEnd = CandidateBase + CandidateEntry->SizeOfImage;

            /* Make sure this entry isn't unloading */
            if (!CandidateEntry->InMemoryOrderLinks.Flink) continue;

            /* Check if our regions are colliding */
            if ((ImageBase >= CandidateBase && ImageBase <= CandidateEnd) ||
                (ImageEnd >= CandidateBase && ImageEnd <= CandidateEnd) ||
                (CandidateBase >= ImageBase && CandidateBase <= ImageEnd))
            {
                /* Found who is overlapping */
                OverlapDllFound = TRUE;
                OverlapDll = CandidateEntry->FullDllName;
                break;
            }
        }

        /* Check if we found the DLL overlapping with us */
        if (!OverlapDllFound)
        {
            /* It's not another DLL, it's memory already here */
            RtlInitUnicodeString(&OverlapDll, L"Dynamically Allocated Memory");
        }

        DPRINT1("Overlapping DLL: %wZ\n", &OverlapDll);

        if (ApplyRelocs)
        {
            UNICODE_STRING IllegalDll;

            /* See if this is an Illegal DLL - IE: user32 and kernel32 */
            RtlInitUnicodeString(&IllegalDll, L"user32.dll");
            if (RtlEqualUnicodeString(&LoadContext->Module->BaseDllName, &IllegalDll, TRUE))
            {
                /* Can't relocate user32 */
                RelocatableDll = FALSE;
            }
            else
            {
                RtlInitUnicodeString(&IllegalDll, L"kernel32.dll");
                if (RtlEqualUnicodeString(&LoadContext->Module->BaseDllName, &IllegalDll, TRUE))
                {
                    /* Can't relocate kernel32 */
                    RelocatableDll = FALSE;
                }
            }

            /* Known DLLs are not allowed to be relocated */
            if (LoadContext->Flags.KnownDll && !RelocatableDll)
            {
                ULONG Response;
                ULONG_PTR HardErrorParameters[2];

                /* Setup for hard error */
                HardErrorParameters[0] = (ULONG_PTR)&IllegalDll;
                HardErrorParameters[1] = (ULONG_PTR)&OverlapDll;

                /* Raise the error */
                ZwRaiseHardError(STATUS_ILLEGAL_DLL_RELOCATION,
                    2,
                    3,
                    HardErrorParameters,
                    OptionOk,
                    &Response);

                /* If initializing, increase the error count */
                if (LdrInitState != 3) LdrpFatalHardErrorCount++;

                /* Don't do relocation */
                Status = STATUS_CONFLICTING_ADDRESSES;
                goto FailRelocate;
            }

            /* Do the relocation */
            Status = LdrpRelocateImage(LoadContext->Module->DllBase,
                NULL,
                NtHeaders,
                STATUS_SUCCESS,
                STATUS_CONFLICTING_ADDRESSES,
                STATUS_INVALID_IMAGE_FORMAT);

FailRelocate:
            /* Handle any kind of failure */
            if (!NT_SUCCESS(Status))
            {
                /* Remove it from the lists */
                RemoveEntryList(&LoadContext->Module->InLoadOrderLinks);
                RemoveEntryList(&LoadContext->Module->InMemoryOrderLinks);
                RemoveEntryList(&LoadContext->Module->HashLinks);

                /* Unmap it, clear the entry */
                NtUnmapViewOfSection(NtCurrentProcess(), LoadContext->Module->DllBase);
            }

            /* Show debug message */
            if (LdrpDebugFlags.LogTrace)
            {
                DPRINT1("LDR: Fixups %successfully re-applied @ " LDR_HEXPTR_FMT "\n",
                    NT_SUCCESS(Status) ? "s" : "uns", LoadContext->Module->DllBase);
            }
        }
        else
        {
            /* Not a DLL, or no relocation needed */
            Status = STATUS_SUCCESS;

            /* Show debug message */
            if (LdrpDebugFlags.LogTrace)
            {
                DPRINT1("LDR: Fixups won't be re-applied to non-Dll @ " LDR_HEXPTR_FMT "\n", LoadContext->Module->DllBase);
            }
        }
    }

    /* Check if this is an SMP Machine and a DLL */
    if (NT_SUCCESS(Status) && LoadContext->Module->ImageDll)
    {
        Status = LdrpValidateImageForMp(LoadContext->Module);
    }

    return Status;
}
