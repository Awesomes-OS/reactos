#include <ldrp.h>

VOID
NTAPI
LdrpInsertMemoryTableEntry(IN PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    PPEB_LDR_DATA PebData = NtCurrentPeb()->Ldr;
    ULONG HashIndex;

    if (LdrEntry->InLegacyLists)
    {
        DPRINT1("LDR: %s(\"%wZ\") Already present in PEB LDR module lists\n", __FUNCTION__, &LdrEntry->BaseDllName);
        return;
    }

    if (!LdrEntry->BaseNameHashValue)
        LdrEntry->BaseNameHashValue = LdrpHashUnicodeString(&LdrEntry->BaseDllName);

    // Insert into LDR hash table
    HashIndex = LDR_GET_HASH_ENTRY(LdrEntry->BaseNameHashValue);
    InsertTailList(&LdrpHashTable[HashIndex], &LdrEntry->HashLinks);

    // Insert into PEB LDR module lists
    RtlpCheckListEntry(&PebData->InLoadOrderModuleList);
    RtlpCheckListEntry(&PebData->InMemoryOrderModuleList);

    if (LdrEntry == LdrpImageEntry)
    {
        InsertHeadList(&PebData->InLoadOrderModuleList, &LdrEntry->InLoadOrderLinks);
        InsertHeadList(&PebData->InMemoryOrderModuleList, &LdrEntry->InMemoryOrderLinks);
    }
    else
    {
        InsertTailList(&PebData->InLoadOrderModuleList, &LdrEntry->InLoadOrderLinks);
        InsertTailList(&PebData->InMemoryOrderModuleList, &LdrEntry->InMemoryOrderLinks);
    }
    
    RtlpCheckListEntry(&PebData->InLoadOrderModuleList);
    RtlpCheckListEntry(&PebData->InMemoryOrderModuleList);

    LdrEntry->InLegacyLists = TRUE;
}