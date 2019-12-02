#include <ldrp.h>
#include <iofuncs.h>
#include <mmfuncs.h>
#include <obfuncs.h>

NTSTATUS
NTAPI
LdrpFindLoadedDllByMappingFile(PUNICODE_STRING NtPathName, PLDR_DATA_TABLE_ENTRY* LdrEntry, LDR_DDAG_STATE* DdagState)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE FileHandle, SectionHandle;
    IO_STATUS_BLOCK Iosb;
    PVOID ViewBase = NULL;
    SIZE_T ViewSize = 0;
    PIMAGE_NT_HEADERS NtHeader;

    /* Initialize object attributes and open it */
    InitializeObjectAttributes(&ObjectAttributes,
                               NtPathName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    NTSTATUS Status = NtOpenFile(&FileHandle,
                                 SYNCHRONIZE | FILE_READ_DATA,
                                 &ObjectAttributes,
                                 &Iosb,
                                 FILE_SHARE_READ | FILE_SHARE_DELETE,
                                 FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);

    /* Free NT path name */
    RtlFreeHeap(RtlGetProcessHeap(), 0, NtPathName->Buffer);

    if (!NT_SUCCESS(Status))
        return Status;

    /* Create a section for this file */
    Status = NtCreateSection(&SectionHandle,
                             SECTION_MAP_READ,
                             NULL,
                             NULL,
                             PAGE_READONLY,
                             SEC_COMMIT,
                             FileHandle);

    /* Close file handle */
    NtClose(FileHandle);

    if (!NT_SUCCESS(Status))
        return Status;

    /* Map view of this section */
    Status = ZwMapViewOfSection(SectionHandle,
                                NtCurrentProcess(),
                                &ViewBase,
                                0,
                                0,
                                NULL,
                                &ViewSize,
                                ViewShare,
                                0,
                                PAGE_READONLY);

    NtClose(SectionHandle);

    /* If section mapping failed - return failure */
    if (!NT_SUCCESS(Status))
        return Status;

    /* Get pointer to the NT header of this section */
    Status = RtlImageNtHeaderEx(0, ViewBase, ViewSize, &NtHeader);

    if (NT_SUCCESS(Status))
    {
        /* Perform actual matching of loaded images against the NT header */
        Status = LdrpFindLoadedDllByMapping(ViewBase, NtHeader, LdrEntry, DdagState);
    }

    NtUnmapViewOfSection(NtCurrentProcess(), ViewBase);

    return Status;
}
