#include <ldrp.h>
#include <ndk/obfuncs.h>

BOOLEAN
NTAPI
LdrpCheckKnownDllFullPath(IN PUNICODE_STRING DllName, IN OUT PUNICODE_STRING BaseDllName)
{
    if (!LdrpKnownDllPath.Length)
        return FALSE;

    for (WCHAR* pChar = DllName->Buffer + DllName->Length / sizeof(WCHAR) - 1; pChar >= DllName->Buffer; pChar--)
    {
        BaseDllName->Buffer = pChar;
        const WCHAR c = *pChar;
        if (c == '\\' || c == '/')
        {
            BaseDllName->Buffer = ++pChar;
            break;
        }
    }

    const SIZE_T Length = BaseDllName->Buffer - DllName->Buffer;
    BaseDllName->Length = DllName->Length - Length;
    BaseDllName->MaximumLength = DllName->MaximumLength - Length;

    UNICODE_STRING Directory;
    Directory.Buffer = DllName->Buffer;
    Directory.MaximumLength = Length;
    Directory.Length = Directory.MaximumLength - sizeof(WCHAR);

    return RtlEqualUnicodeString(&Directory, &LdrpKnownDllPath, TRUE);
}

NTSTATUS
NTAPI
LdrpLoadKnownDll(IN PLDRP_LOAD_CONTEXT LoadContext)
{
    HANDLE SectionHandle = NULL;

    ASSERT(LoadContext);

    if (LoadContext->Flags.HasFullPath)
    {
        UNICODE_STRING BaseDllName = {0};

        if (!LdrpCheckKnownDllFullPath(&LoadContext->DllName, &BaseDllName))
            return STATUS_DLL_NOT_FOUND;

        LoadContext->DllName = BaseDllName;
    }
    else
    {
        if (!LoadContext->Flags.BaseNameOnly)
            return STATUS_DLL_NOT_FOUND;
    }

    /* Try to find a Known DLL */
    NTSTATUS Status = LdrpFindKnownDll(&LoadContext->DllName,
                                       &LoadContext->Entry->BaseDllName,
                                       &LoadContext->Entry->FullDllName,
                                       &SectionHandle);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    LoadContext->Flags.KnownDll = TRUE;

    const ULONG32 BaseNameHashValue = LdrpHashUnicodeString(&LoadContext->Entry->BaseDllName);
    LoadContext->Entry->BaseNameHashValue = BaseNameHashValue;

    PLDR_DATA_TABLE_ENTRY LdrEntry = NULL;

    RtlAcquireSRWLockExclusive(&LdrpModuleDatatableLock);

    Status = LdrpFindLoadedDllByNameLockHeld(&LoadContext->Entry->BaseDllName,
                                             &LoadContext->Entry->FullDllName,
                                             LoadContext->Flags,
                                             BaseNameHashValue,
                                             &LdrEntry);

    RtlReleaseSRWLockExclusive(&LdrpModuleDatatableLock);

    if (LdrEntry)
    {
        LdrpLoadContextReplaceModule(LoadContext, LdrEntry);
    }
    else
    {
        Status = LdrpMapDllWithSectionHandle(LoadContext, SectionHandle);
    }

    NtClose(SectionHandle);

    return Status;
}
