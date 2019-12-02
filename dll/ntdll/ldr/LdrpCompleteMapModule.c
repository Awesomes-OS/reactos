#include <ldrp.h>
#include <ndk/exfuncs.h>
#include <ndk/mmfuncs.h>

NTSTATUS
NTAPI
LdrpCompleteMapModule(PLDRP_LOAD_CONTEXT LoadContext, PIMAGE_NT_HEADERS NtHeaders, NTSTATUS ImageStatus, SIZE_T ViewSize)
{
    PPEB Peb = NtCurrentPeb();
    BOOLEAN OverlapDllFound = FALSE;
    UNICODE_STRING OverlapDll;
    BOOLEAN RelocatableDll = TRUE;
    NTSTATUS Status = STATUS_SUCCESS;

    /* Check if we loaded somewhere else */
    if (ImageStatus == STATUS_IMAGE_NOT_AT_BASE || ImageStatus == STATUS_IMAGE_AT_DIFFERENT_BASE)
    {
        ULONG_PTR ImageBase, ImageEnd;
        PLDR_DATA_TABLE_ENTRY CandidateEntry;
        ULONG_PTR CandidateBase, CandidateEnd;
        PLIST_ENTRY ListHead, NextEntry;

        BOOL ApplyRelocs = LoadContext->Entry->ImageDll; /* Are we dealing with a DLL? */

        /* Find our region */
        ImageBase = (ULONG_PTR)NtHeaders->OptionalHeader.ImageBase;
        ImageEnd = ImageBase + ViewSize;

        DPRINT1("LDR: LdrpMapDll Relocating Image Name %wZ (%p-%p -> %p)\n",
            LoadContext->Entry->BaseDllName, (PVOID)ImageBase, (PVOID)ImageEnd, LoadContext->Entry->DllBase);

        /* Scan all the modules */
        ListHead = &Peb->Ldr->InLoadOrderModuleList;
        NextEntry = ListHead->Flink;
        while (NextEntry != ListHead)
        {
            /* Get the entry */
            CandidateEntry = CONTAINING_RECORD(NextEntry,
                LDR_DATA_TABLE_ENTRY,
                InLoadOrderLinks);
            NextEntry = NextEntry->Flink;

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

        if (LoadContext->Entry->CorImage)
        {
            if (LoadContext->Entry->CorILOnly)
                ApplyRelocs = FALSE;
            else if (!NT_SUCCESS(Status = LdrpCorValidateImage(LoadContext->Entry->DllBase, NULL)))
                ApplyRelocs = FALSE;
        }

        if (ApplyRelocs)
        {
            UNICODE_STRING IllegalDll;

            /* See if this is an Illegal DLL - IE: user32 and kernel32 */
            RtlInitUnicodeString(&IllegalDll, L"user32.dll");
            if (RtlEqualUnicodeString(&LoadContext->Entry->BaseDllName, &IllegalDll, TRUE))
            {
                /* Can't relocate user32 */
                RelocatableDll = FALSE;
            }
            else
            {
                RtlInitUnicodeString(&IllegalDll, L"kernel32.dll");
                if (RtlEqualUnicodeString(&LoadContext->Entry->BaseDllName, &IllegalDll, TRUE))
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
                if (LdrpInLdrInit) LdrpFatalHardErrorCount++;

                /* Don't do relocation */
                Status = STATUS_CONFLICTING_ADDRESSES;
                goto FailRelocate;
            }

            /* Do the relocation */
            Status = LdrpRelocateImage(LoadContext->Entry->DllBase,
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
                RemoveEntryList(&LoadContext->Entry->InLoadOrderLinks);
                RemoveEntryList(&LoadContext->Entry->InMemoryOrderLinks);
                RemoveEntryList(&LoadContext->Entry->HashLinks);

                /* Unmap it, clear the entry */
                NtUnmapViewOfSection(NtCurrentProcess(), LoadContext->Entry->DllBase);
            }

            /* Show debug message */
            if (ShowSnaps)
            {
                DPRINT1("LDR: Fixups %successfully re-applied @ %p\n",
                    NT_SUCCESS(Status) ? "s" : "uns", LoadContext->Entry->DllBase);
            }
        }
        else
        {
            /* Not a DLL, or no relocation needed */
            Status = STATUS_SUCCESS;

            /* Show debug message */
            if (ShowSnaps)
            {
                DPRINT1("LDR: Fixups won't be re-applied to non-Dll @ %p\n", LoadContext->Entry->DllBase);
            }
        }
    }

    /* Check if this is an SMP Machine and a DLL */
    if ((LdrpNumberOfProcessors > 1) && LoadContext->Entry->ImageDll)
    {
        /* Validate the image for MP */
        LdrpValidateImageForMp(LoadContext->Entry);
    }

    return Status;
}
