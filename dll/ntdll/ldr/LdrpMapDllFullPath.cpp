#include <ldrp.h>

NTSTATUS
NTAPI
LdrpMapDllFullPath(IN PLDRP_LOAD_CONTEXT LoadContext)
{
    LDRP_ASSERT_LOAD_CONTEXT(LoadContext);

    NTSTATUS Status;
    LDRP_UNICODE_STRING_BUNDLE NtName;

    LdrpCreateUnicodeStringBundle(NtName);

    Status = LdrpResolveDllName(&LoadContext->DllName, &NtName, &LoadContext->Module->BaseDllName,
                                &LoadContext->Module->FullDllName, LoadContext->Flags);

    if (NT_SUCCESS(Status))
    {
        const ULONG32 BaseNameHashValue = LdrpHashUnicodeString(&LoadContext->Module->BaseDllName);
        LoadContext->Module->BaseNameHashValue = BaseNameHashValue;

        PLDR_DATA_TABLE_ENTRY Module = NULL;
        Status = LdrpFindExistingModule(&LoadContext->Module->BaseDllName, &LoadContext->Module->FullDllName,
                                        LoadContext->Flags, BaseNameHashValue, &Module);

        if (NT_SUCCESS(Status) && Module)
        {
            LdrpLoadContextReplaceModule(LoadContext, Module);
        }
        else
        {
            Status = LdrpMapDllNtFileName(LoadContext, &NtName.String);
            if (Status == STATUS_IMAGE_MACHINE_TYPE_MISMATCH)
                Status = STATUS_INVALID_IMAGE_FORMAT;
        }
    }

    LdrpFreeUnicodeStringBundle(NtName);
    return Status;
}
