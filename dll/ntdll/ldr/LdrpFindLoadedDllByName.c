#include <ldrp.h>

NTSTATUS
NTAPI
LdrpFindLoadedDllByNameLockHeld(IN PUNICODE_STRING BaseDllName,
                                IN PUNICODE_STRING FullDllName OPTIONAL,
                                IN LDRP_LOAD_CONTEXT_FLAGS LoadContextFlags OPTIONAL,
                                IN ULONG32 BaseNameHashValue,
                                OUT PLDR_DATA_TABLE_ENTRY* LdrEntry)
{
    ULONG HashIndex;
    PLIST_ENTRY ListHead, ListEntry;

    /* Get hash index */
    HashIndex = LDR_GET_HASH_ENTRY(BaseNameHashValue);

    /* Traverse that list */
    ListHead = &LdrpHashTable[HashIndex];
    ListEntry = ListHead->Flink;
    while (ListEntry != ListHead)
    {
        /* Get the current entry */
        PLDR_DATA_TABLE_ENTRY CurEntry = CONTAINING_RECORD(ListEntry, LDR_DATA_TABLE_ENTRY, HashLinks);

        /* Check base name of that module */
        if (BaseDllName && RtlEqualUnicodeString(BaseDllName, &CurEntry->BaseDllName, TRUE))
        {
            /* It matches, return it */
            *LdrEntry = CurEntry;

            if (CurEntry->DdagNode->LoadCount != LDR_LOADCOUNT_MAX && !CurEntry->ProcessStaticImport)
            {
                InterlockedIncrement((LONG*)&CurEntry->ReferenceCount);
            }

            DPRINT("LDR: %s(\"%wZ\", \"%wZ\", %#x, %u, %p) -> SUCCESS [BaseDllName]\n",
                   __FUNCTION__, BaseDllName, FullDllName, LoadContextFlags, BaseNameHashValue, LdrEntry);
            return STATUS_SUCCESS;
        }

        /* Check full name of that module */
        if (FullDllName && RtlEqualUnicodeString(FullDllName, &CurEntry->FullDllName, TRUE))
        {
            /* It matches, return it */
            *LdrEntry = CurEntry;

            if (CurEntry->DdagNode->LoadCount != LDR_LOADCOUNT_MAX && !CurEntry->ProcessStaticImport)
            {
                InterlockedIncrement((LONG*)&CurEntry->ReferenceCount);
            }

            DPRINT("LDR: %s(\"%wZ\", \"%wZ\", %#x, %u, %p) -> SUCCESS [FullDllName]\n",
                   __FUNCTION__, BaseDllName, FullDllName, LoadContextFlags, BaseNameHashValue, LdrEntry);
            return STATUS_SUCCESS;
        }

        /* Advance to the next entry */
        ListEntry = ListEntry->Flink;
    }

    DPRINT("LDR: %s(\"%wZ\", \"%wZ\", %#x, %u, %p) -> STATUS_DLL_NOT_FOUND\n",
           __FUNCTION__, BaseDllName, FullDllName, LoadContextFlags, BaseNameHashValue, LdrEntry);

    return STATUS_DLL_NOT_FOUND;
}


NTSTATUS
NTAPI
LdrpFindLoadedDllByName(IN PUNICODE_STRING BaseDllName OPTIONAL,
                        IN PUNICODE_STRING FullDllName OPTIONAL,
                        IN LDRP_LOAD_CONTEXT_FLAGS LoadContextFlags OPTIONAL,
                        OUT PLDR_DATA_TABLE_ENTRY* LdrEntry,
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

    const NTSTATUS Result = LdrpFindLoadedDllByNameLockHeld(BaseDllName, FullDllName, LoadContextFlags, Hash, LdrEntry);
    
    RtlReleaseSRWLockExclusive(&LdrpModuleDatatableLock);

    if (NT_SUCCESS(Result) && DdagState)
        *DdagState = (*LdrEntry)->DdagNode->State;

    return Result;
}
