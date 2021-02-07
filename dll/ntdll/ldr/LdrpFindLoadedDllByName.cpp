#include <ldrp.h>

NTSTATUS
NTAPI
LdrpFindLoadedDllByNameLockHeld(IN PUNICODE_STRING BaseDllName,
                                IN PUNICODE_STRING FullDllName OPTIONAL,
                                IN LDRP_LOAD_CONTEXT_FLAGS LoadContextFlags OPTIONAL,
                                IN ULONG32 BaseNameHashValue,
                                OUT PLDR_DATA_TABLE_ENTRY* Module)
{
    /* Get hash index */
    const ULONG HashIndex = LDR_GET_HASH_ENTRY(BaseNameHashValue);

    /* Traverse that list */
    const PLIST_ENTRY ListHead = &LdrpHashTable[HashIndex];

    for (PLIST_ENTRY ListEntry = ListHead->Flink; ListEntry != ListHead; ListEntry = ListEntry->Flink)
    {
        /* Get the current entry */
        PLDR_DATA_TABLE_ENTRY CurEntry = CONTAINING_RECORD(ListEntry, LDR_DATA_TABLE_ENTRY, HashLinks);

        /* Check base name of that module */
        if (BaseDllName && RtlEqualUnicodeString(BaseDllName, &CurEntry->BaseDllName, TRUE))
        {
            /* It matches, return it */
            *Module = CurEntry;

            if (!LDRP_MODULE_PINNED(CurEntry))
            {
                InterlockedIncrement((LONG*)&CurEntry->ReferenceCount);
            }

            DPRINT("LDR: %s(\"%wZ\", \"%wZ\", %#x, %u, " LDR_HEXPTR_FMT ") -> SUCCESS [BaseDllName]\n",
                   __FUNCTION__, BaseDllName, FullDllName, LoadContextFlags.Flags, BaseNameHashValue, Module);
            return STATUS_SUCCESS;
        }

        /* Check full name of that module */
        if (FullDllName && RtlEqualUnicodeString(FullDllName, &CurEntry->FullDllName, TRUE))
        {
            /* It matches, return it */
            *Module = CurEntry;

            if (!LDRP_MODULE_PINNED(CurEntry))
            {
                InterlockedIncrement((LONG*)&CurEntry->ReferenceCount);
            }

            DPRINT("LDR: %s(\"%wZ\", \"%wZ\", %#x, %u, " LDR_HEXPTR_FMT ") -> SUCCESS [FullDllName]\n",
                   __FUNCTION__, BaseDllName, FullDllName, LoadContextFlags.Flags, BaseNameHashValue, Module);
            return STATUS_SUCCESS;
        }
    }

    DPRINT("LDR: %s(\"%wZ\", \"%wZ\", %#x, %u, " LDR_HEXPTR_FMT ") -> STATUS_DLL_NOT_FOUND\n",
           __FUNCTION__, BaseDllName, FullDllName, LoadContextFlags.Flags, BaseNameHashValue, Module);

    return STATUS_DLL_NOT_FOUND;
}


NTSTATUS
NTAPI
LdrpFindLoadedDllByName(IN PUNICODE_STRING BaseDllName OPTIONAL,
                        IN PUNICODE_STRING FullDllName OPTIONAL,
                        IN LDRP_LOAD_CONTEXT_FLAGS LoadContextFlags OPTIONAL,
                        OUT PLDR_DATA_TABLE_ENTRY* Module,
                        OUT LDR_DDAG_STATE* DdagState OPTIONAL)
{
    UNICODE_STRING FoundBaseName;
    ULONG32 Hash;

    ASSERT(BaseDllName || FullDllName);

    if (!BaseDllName)
    {
        WCHAR* pChar;

        for (pChar = FullDllName->Buffer + FullDllName->Length / sizeof(WCHAR) - 1; pChar >= FullDllName->Buffer; pChar--)
        {
            const WCHAR c = *pChar;
            if (c == '\\' || c == '/')
            {
                ++pChar;
                break;
            }
        }

        RtlInitUnicodeString(&FoundBaseName, pChar);
        BaseDllName = &FoundBaseName;
    }

    Hash = LdrpHashUnicodeString(BaseDllName);

    RtlAcquireSRWLockExclusive(&LdrpModuleDatatableLock);

    const NTSTATUS Result = LdrpFindLoadedDllByNameLockHeld(BaseDllName, FullDllName, LoadContextFlags, Hash, Module);
    
    RtlReleaseSRWLockExclusive(&LdrpModuleDatatableLock);

    if (NT_SUCCESS(Result) && DdagState)
        *DdagState = (*Module)->DdagNode->State;

    return Result;
}
