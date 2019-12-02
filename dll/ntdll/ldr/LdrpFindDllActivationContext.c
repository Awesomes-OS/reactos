#include <ldrp.h>

NTSTATUS
NTAPI
LdrpFindDllActivationContext(PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (!LdrpManifestProberRoutine)
        return STATUS_SUCCESS;

    if (LdrEntry == LdrpImageEntry && NtCurrentPeb()->ActivationContextData)
        return STATUS_SUCCESS;

    PACTIVATION_CONTEXT ActivationContext;

    PWSTR FullDllName = LdrEntry->FullDllName.Buffer;

    if (LdrEntry == LdrpImageEntry)
    {
        // \??\C:\ReactOS\notepad.exe
        if (FullDllName[0] == L'\\'
            && FullDllName[1] == L'?'
            && FullDllName[2] == L'?'
            && FullDllName[3] == L'\\'
            && FullDllName[4] != UNICODE_NULL
            && FullDllName[5] == L':'
            && FullDllName[6] == L'\\')
        {
            FullDllName += 4;
            // C:\ReactOS\notepad.exe
        }
    }

    // Probe the DLL for its manifest.
    Status = LdrpManifestProberRoutine(LdrEntry->DllBase, FullDllName, &ActivationContext);

    if (Status == STATUS_NO_SUCH_FILE
        || Status == STATUS_NOT_IMPLEMENTED
        || Status == STATUS_NOT_SUPPORTED
        || Status == STATUS_RESOURCE_DATA_NOT_FOUND
        || Status == STATUS_RESOURCE_TYPE_NOT_FOUND
        || Status == STATUS_RESOURCE_NAME_NOT_FOUND
        || Status == STATUS_RESOURCE_LANG_NOT_FOUND)
    {
        if (LdrpDebugFlags.LogDebug)
        {
            DPRINT1("LDR: %s([\"%wZ\"]): Probing for the manifest failed with status 0x%08lX\n",
                    __FUNCTION__, &LdrEntry->FullDllName, Status);
        }

        Status = STATUS_SUCCESS;
    }

    if (ActivationContext)
    {
        if (LdrEntry->EntryPointActivationContext)
            RtlReleaseActivationContext(LdrEntry->EntryPointActivationContext);

        LdrEntry->EntryPointActivationContext = ActivationContext;
    }

    if (!NT_SUCCESS(Status))
    {
        if (LdrpDebugFlags.LogDebug)
        {
            DPRINT1("LDR: %s([\"%wZ\"]): Querying the active activation context failed with status 0x%08lX\n",
                    __FUNCTION__, &LdrEntry->FullDllName, Status);
        }

        if (LdrpDebugFlags.BreakInDebugger)
            __debugbreak();
    }

    return Status;
}
