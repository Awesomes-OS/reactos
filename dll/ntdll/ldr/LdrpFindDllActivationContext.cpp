#include <ldrp.h>

NTSTATUS
NTAPI
LdrpFindDllActivationContext(PLDR_DATA_TABLE_ENTRY Module)
{
    LDR_FUNC(NTSTATUS, Module)
        NTSTATUS Status;

        ASSERT_EQ(Module->DdagNode->State, LdrModulesMapped);

        if (!LdrpManifestProberRoutine)
            return STATUS_SUCCESS;

        if (Module == LdrpImageEntry && NtCurrentPeb()->ActivationContextData)
            return STATUS_SUCCESS;

        PACTIVATION_CONTEXT ActivationContext;

        PWSTR FullDllName = Module->FullDllName.Buffer;

        if (Module == LdrpImageEntry)
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
        Status = LdrpManifestProberRoutine(Module->DllBase, FullDllName, &ActivationContext);

        if (Status == STATUS_NO_SUCH_FILE
            || Status == STATUS_NOT_IMPLEMENTED
            || Status == STATUS_NOT_SUPPORTED
            || Status == STATUS_RESOURCE_DATA_NOT_FOUND
            || Status == STATUS_RESOURCE_TYPE_NOT_FOUND
            || Status == STATUS_RESOURCE_NAME_NOT_FOUND
            || Status == STATUS_RESOURCE_LANG_NOT_FOUND)
        {
            LDR_DEBUG("Probing for the manifest failed with status " LDR_HEX32_FMT, Status);

            Status = STATUS_SUCCESS;
        }

        if (ActivationContext)
        {
            ASSERT_SUCCESS_OR_IGNORE(Status);

            if (Module->EntryPointActivationContext)
                RtlReleaseActivationContext(Module->EntryPointActivationContext);

            Module->EntryPointActivationContext = ActivationContext;
        }

        if (!NT_SUCCESS(Status))
        {
            LDR_WARNING("Querying the active activation context failed with status " LDR_HEX32_FMT, Status);

            if (LdrpDebugFlags.BreakInDebugger)
                __debugbreak();
        }

        return Status;
    LDR_FUNC_END_RETURN
}
