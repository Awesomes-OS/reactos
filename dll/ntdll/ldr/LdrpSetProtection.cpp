#include <ldrp.h>

EXTERN_C
{

#include <ndk/mmfuncs.h>
#include <ndk/kefuncs.h>

}

NTSTATUS
NTAPI
LdrpSetProtection(PVOID ViewBase,
                  BOOLEAN Restore)
{
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_SECTION_HEADER Section;
    NTSTATUS Status;
    PVOID SectionBase;
    SIZE_T SectionSize;
    ULONG NewProtection, OldProtection, i;

    /* Get the NT headers */
    NtHeaders = RtlImageNtHeader(ViewBase);
    if (!NtHeaders) return STATUS_INVALID_IMAGE_FORMAT;

    /* Compute address of the first section header */
    Section = IMAGE_FIRST_SECTION(NtHeaders);

    /* Go through all sections */
    for (i = 0; i < NtHeaders->FileHeader.NumberOfSections; i++)
    {
        /* Check for read-only non-zero section */
        if ((Section->SizeOfRawData) &&
            !(Section->Characteristics & IMAGE_SCN_MEM_WRITE))
        {
            /* Check if we are setting or restoring protection */
            if (Restore)
            {
                /* Set it to either EXECUTE or READONLY */
                if (Section->Characteristics & IMAGE_SCN_MEM_EXECUTE)
                {
                    if (Section->Characteristics & IMAGE_SCN_MEM_READ)
                        NewProtection = PAGE_EXECUTE_READ;
                    else
                        NewProtection = PAGE_EXECUTE;
                }
                else
                {
                    NewProtection = PAGE_READONLY;
                }

                /* Add PAGE_NOCACHE if needed */
                if (Section->Characteristics & IMAGE_SCN_MEM_NOT_CACHED)
                {
                    NewProtection |= PAGE_NOCACHE;
                }
            }
            else
            {
                /* Enable write access */
                NewProtection = PAGE_READWRITE;
            }

            /* Get the section VA */
            SectionBase = (PVOID)((ULONG_PTR)ViewBase + Section->VirtualAddress);
            SectionSize = Section->SizeOfRawData;
            if (SectionSize)
            {
                /* Set protection */
                Status = ZwProtectVirtualMemory(NtCurrentProcess(),
                                                &SectionBase,
                                                &SectionSize,
                                                NewProtection,
                                                &OldProtection);
                if (!NT_SUCCESS(Status)) return Status;
            }
        }

        /* Move to the next section */
        Section++;
    }

    /* Flush instruction cache if necessary */
    if (Restore) ZwFlushInstructionCache(NtCurrentProcess(), NULL, 0);
    return STATUS_SUCCESS;
}
