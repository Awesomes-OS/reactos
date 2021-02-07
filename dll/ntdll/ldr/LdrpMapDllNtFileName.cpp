#include <ldrp.h>
#include <ndk/obfuncs.h>

NTSTATUS
NTAPI
LdrpMapDllNtFileName(IN PLDRP_LOAD_CONTEXT LoadContext,
                     IN PUNICODE_STRING NtPathDllName)
{
    LDRP_ASSERT_LOAD_CONTEXT(LoadContext);
    ASSERT(NtPathDllName);

    ASSERT(LoadContext->Module->LoadContext == LoadContext);

    HANDLE MappedFileHandle = NULL, MappedSectionHandle = NULL;

    if (LdrpCheckForRetryLoading(LoadContext, FALSE))
        return STATUS_RETRY;

    /* Create a section for this DLL */
    NTSTATUS Status = LdrpCreateDllSection(NtPathDllName,
                                           LoadContext->Flags,
                                           &MappedFileHandle,
                                           &MappedSectionHandle);

    /* Check if we failed */
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = LdrpMapDllWithSectionHandle(LoadContext, MappedSectionHandle);

    // Preserve handles within LoadContext when have enclaves && map succeeded

    NtClose(MappedSectionHandle);
    NtClose(MappedFileHandle);

    return Status;
}
