#include <ldrp.h>
#include <ndk/iofuncs.h>

NTSTATUS
NTAPI
LdrpGetNtPathFromDosPath(IN PUNICODE_STRING DosPath,
                         OUT PLDRP_UNICODE_STRING_BUNDLE NtPath)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    FILE_BASIC_INFORMATION BasicInformation;

    ASSERT(DosPath);
    ASSERT(NtPath);

    RTLP_DosPathNameToRelativeNtPathName_FLAGS Flags = { 0 };

    // All callers pass full paths in this function
    Flags.InputIsFullPath = TRUE;

    UNICODE_STRING DynamicBuffer = {0};
    PUNICODE_STRING ResultNtPath = NULL;
    NTSTATUS Status = RtlpDosPathNameToRelativeNtPathName(Flags,
                                                          DosPath,
                                                          &NtPath->String,
                                                          &DynamicBuffer,
                                                          &ResultNtPath,
                                                          NULL,
                                                          NULL);

    if (!NT_SUCCESS(Status))
        return Status;

    if (ResultNtPath == &DynamicBuffer)
    {
        LdrpFreeUnicodeStringBundlePtr(NtPath);
        NtPath->String = DynamicBuffer;
    }

    /* Initialize the object attributes */
    InitializeObjectAttributes(&ObjectAttributes,
                               &NtPath->String,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Query the attributes */
    Status = ZwQueryAttributesFile(&ObjectAttributes, &BasicInformation);

    return Status;
}
