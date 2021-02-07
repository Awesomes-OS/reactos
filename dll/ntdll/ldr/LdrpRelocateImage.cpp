#include <ldrp.h>


ULONG
NTAPI
LdrpRelocateImage(
    IN PVOID BaseAddress,
    IN PCCH LoaderName,
    IN PIMAGE_NT_HEADERS NtHeaders,
    IN ULONG Success,
    IN ULONG Conflict,
    IN ULONG Invalid)
{
    if (LdrpIsILOnlyImage(BaseAddress))
        return Success;

    /* Check if relocs were stripped */
    if (NtHeaders->FileHeader.Characteristics & IMAGE_FILE_RELOCS_STRIPPED)
        return Success;

    ULONG RelocDataSize;
    /* Get the relocation data */
    const PVOID RelocData = RtlImageDirectoryEntryToData(BaseAddress,
                                                   TRUE,
                                                   IMAGE_DIRECTORY_ENTRY_BASERELOC,
                                                   &RelocDataSize);

    /* Does the DLL not have any? */
    if (!RelocData || !RelocDataSize)
    {
        return Success;
    }

    return LdrpProtectAndRelocateImage(BaseAddress, LoaderName, Success, Conflict, Invalid);
}