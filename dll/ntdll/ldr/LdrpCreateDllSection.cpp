#include <ldrp.h>

EXTERN_C
{

#include <ndk/mmfuncs.h>
#include <ndk/iofuncs.h>
#include <ndk/exfuncs.h>
#include <ndk/obfuncs.h>

}

NTSTATUS
NTAPI
LdrpCreateDllSection(IN PUNICODE_STRING FullName,
                     IN LDRP_LOAD_CONTEXT_FLAGS LoaderFlags,
                     OUT PHANDLE FileHandle,
                     OUT PHANDLE SectionHandle)
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    SECTION_IMAGE_INFORMATION SectionImageInfo;

    /* Check if we don't already have a handle */
    if (!*FileHandle)
    {
        /* Create the object attributes */
        InitializeObjectAttributes(&ObjectAttributes,
                                   FullName,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        for (int i = 0; i <= 1; ++i)
        {
            /* Open the DLL */
            Status = NtOpenFile(FileHandle,
                                SYNCHRONIZE | FILE_EXECUTE | FILE_READ_DATA,
                                &ObjectAttributes,
                                &IoStatusBlock,
                                FILE_SHARE_READ | FILE_SHARE_DELETE,
                                FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);

            /* Check if we failed */
            if (NT_SUCCESS(Status))
                break;

            /* Show debug message */
            if (LdrpDebugFlags.LogWarning)
            {
                DPRINT1("NtOpenFile failed; status = 0x%08lX\n",
                        Status);
            }

            /* Make sure to return an expected status code */
            if (Status == STATUS_OBJECT_NAME_NOT_FOUND || Status == STATUS_OBJECT_PATH_NOT_FOUND)
            {
                Status = STATUS_DLL_NOT_FOUND;
                break;
            }

            if (Status != STATUS_ACCESS_DENIED)
                break;
        }
    }

    if (!NT_SUCCESS(Status))
    {
        // Return empty handles
        *SectionHandle = NULL;
        *FileHandle = NULL;
        return Status;
    }

    /* Create a section for the DLL */
    Status = NtCreateSection(SectionHandle,
                             SECTION_MAP_READ | SECTION_MAP_EXECUTE | SECTION_QUERY,
                             NULL,
                             NULL,
                             PAGE_EXECUTE,
                             SEC_IMAGE,
                             *FileHandle);

    /* If mapping failed, raise a hard error */
    if (!NT_SUCCESS(Status))
    {
        /* Forget the handle */
        *SectionHandle = NULL;

        NtClose(*FileHandle);

        if (LdrpDebugFlags.LogWarning)
        {
            DPRINT1("NtCreateSection failed; status = 0x%08lX\n",
                    Status);
        }

        if (Status != STATUS_COMMITMENT_LIMIT && Status != STATUS_NO_MEMORY && Status != STATUS_INSUFFICIENT_RESOURCES)
        {
            ULONG_PTR HardErrorParameters[1];
            ULONG Response;

            /* Give the DLL name */
            HardErrorParameters[0] = (ULONG_PTR)FullName;

            /* Raise the error */
            if (NT_SUCCESS(NtRaiseHardError(STATUS_INVALID_IMAGE_FORMAT,
                1,
                1,
                HardErrorParameters,
                OptionOk,
                &Response)))
            {
                /* Increment the error count */
                if (LdrInitState != 3)
                    LdrpFatalHardErrorCount++;
            }
        }

        return Status;
    }

    /* Check for Safer restrictions */
    if (!LoaderFlags.SystemImage)
    {
        /* Make sure it's executable */
        Status = ZwQuerySection(*SectionHandle,
                                SectionImageInformation,
                                &SectionImageInfo,
                                sizeof(SECTION_IMAGE_INFORMATION),
                                NULL);

        if (NT_SUCCESS(Status))
        {
            /* Bypass the check for .NET images */
            if (!(SectionImageInfo.LoaderFlags & IMAGE_LOADER_FLAGS_COMPLUS))
            {
                /* Check with Safer */
                Status = LdrpCodeAuthzCheckDllAllowed(FullName, *FileHandle);

                if (Status == STATUS_NOT_FOUND)
                    Status = STATUS_SUCCESS;
            }
        }
    }

    if (!NT_SUCCESS(Status))
    {
        NtClose(*SectionHandle);
        NtClose(*FileHandle);

        *SectionHandle = NULL;
        *FileHandle = NULL;
    }

    /* Return status */
    return Status;
}
