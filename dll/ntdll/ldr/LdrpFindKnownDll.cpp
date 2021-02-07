#include <ldrp.h>
#include <reactos/ldrp.h>

EXTERN_C
{

#include <ndk/mmfuncs.h>
#include <ndk/obfuncs.h>

}

NTSTATUS
NTAPI
LdrpFindKnownDll(PUNICODE_STRING DllName,
                 PUNICODE_STRING BaseDllName,
                 PUNICODE_STRING FullDllName,
                 HANDLE* SectionHandle)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Section = NULL;
    NTSTATUS Status;

    DPRINT("LDR: %s(\"%wZ\", ...)\n", __FUNCTION__, DllName);

    /* Check if we have a known dll directory */
    if (!LdrpKnownDllDirectoryHandle)
    {
        Status = STATUS_DLL_NOT_FOUND;
        goto Quickie;
    }

    /* If any of these three params are missing then fail */
    if (!SectionHandle || !FullDllName || !BaseDllName)
        return STATUS_INVALID_PARAMETER;

    /* There are all names, init attributes and open the section */
    InitializeObjectAttributes(&ObjectAttributes,
                               DllName,
                               OBJ_CASE_INSENSITIVE,
                               LdrpKnownDllDirectoryHandle,
                               NULL);

    Status = NtOpenSection(&Section,
                           SECTION_QUERY | SECTION_MAP_READ | SECTION_MAP_EXECUTE,
                           &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
            Status = STATUS_DLL_NOT_FOUND;
        goto Quickie;
    }

    /* Set up FullDllName */
    {
        const ULONG FullLength = LdrpKnownDllPath.Length + DllName->Length + sizeof(WCHAR);
        if (!NT_SUCCESS(Status = LdrpAllocateUnicodeString(FullDllName, FullLength)))
        {
            NtClose(Section);
            goto Quickie;
        }
    }

    ASSERT(NT_SUCCESS(RtlAppendUnicodeStringToString(FullDllName, &LdrpKnownDllPath)));
    ASSERT(NT_SUCCESS(RtlAppendUnicodeToString(FullDllName, L"\\")));

    {
        const auto* const BaseName = (PWSTR)PTR_ADD_OFFSET(FullDllName->Buffer, FullDllName->Length);
        ASSERT(NT_SUCCESS(RtlAppendUnicodeStringToString(FullDllName, DllName)));
        ASSERT(NT_SUCCESS(RtlInitUnicodeStringEx(BaseDllName, BaseName)));
    }

    Status = STATUS_SUCCESS;

Quickie:
    *SectionHandle = Section;

    if (!NT_SUCCESS(Status))
    {
        /* Failure */
        DPRINT("LDR: %s(\"%wZ\", ...) -> Status: 0x%08lX\n",
               __FUNCTION__,
               DllName,
               Status);

        return Status;
    }

    /* Return status */
    return Status;
}
