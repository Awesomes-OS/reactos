#include <ldrp.h>

PIMAGE_IMPORT_DESCRIPTOR
NTAPI
LdrpGetImportDescriptorForSnap(PLDRP_LOAD_CONTEXT LoadContext)
{
    NTSTATUS Status;
    PIMAGE_IMPORT_DESCRIPTOR ImportEntry;
    ULONG IatSize;

    // Get the regular IAT
    Status = RtlImageDirectoryEntryToDataEx(LoadContext->Module->DllBase,
                                            TRUE,
                                            IMAGE_DIRECTORY_ENTRY_IMPORT,
                                            &IatSize,
                                            (PVOID*)&ImportEntry);


    if (!NT_SUCCESS(Status))
        ImportEntry = NULL;

    return ImportEntry;
}
