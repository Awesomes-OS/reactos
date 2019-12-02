#include <ldrp.h>

NTSTATUS
NTAPI
LdrpMapDllFullPath(IN PLDRP_LOAD_CONTEXT LoadContext)
{
    NTSTATUS Status;
    LDRP_UNICODE_STRING_BUNDLE NtName;

    LdrpCreateUnicodeStringBundle(NtName);

    Status = LdrpResolveDllName(&LoadContext->DllName, &NtName, &LoadContext->Entry->BaseDllName, &LoadContext->Entry->FullDllName, LoadContext->Flags);

    if (NT_SUCCESS(Status))
    {
        const ULONG32 BaseNameHashValue = LdrpHashUnicodeString(&LoadContext->Entry->BaseDllName);
        LoadContext->Entry->BaseNameHashValue = BaseNameHashValue;

        PLDR_DATA_TABLE_ENTRY LdrEntry = NULL;
        Status = LdrpFindExistingModule(&LoadContext->Entry->BaseDllName, &LoadContext->Entry->FullDllName, LoadContext->Flags, BaseNameHashValue, &LdrEntry);
        if (NT_SUCCESS(Status) && LdrEntry)
        {
            LdrpLoadContextReplaceModule(LoadContext, LdrEntry);
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