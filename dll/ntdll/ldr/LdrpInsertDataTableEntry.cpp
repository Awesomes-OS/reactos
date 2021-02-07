#include <ldrp.h>

VOID
NTAPI
LdrpInsertDataTableEntry(IN PLDR_DATA_TABLE_ENTRY Module)
{
    PPEB_LDR_DATA PebData = NtCurrentPeb()->Ldr;
    ULONG HashIndex;

    if (Module->InLegacyLists)
    {
        DPRINT1("LDR: %s(\"%wZ\") Already present in PEB LDR module lists\n", __FUNCTION__, &Module->BaseDllName);
        return;
    }

    if (!Module->BaseNameHashValue)
        Module->BaseNameHashValue = LdrpHashUnicodeString(&Module->BaseDllName);

    // Insert into LDR hash table
    HashIndex = LDR_GET_HASH_ENTRY(Module->BaseNameHashValue);
    InsertTailList(&LdrpHashTable[HashIndex], &Module->HashLinks);

    // Insert into PEB LDR module lists
    RtlpCheckListEntry(&PebData->InLoadOrderModuleList);
    RtlpCheckListEntry(&PebData->InMemoryOrderModuleList);

    if (Module == LdrpImageEntry)
    {
        InsertHeadList(&PebData->InLoadOrderModuleList, &Module->InLoadOrderLinks);
        InsertHeadList(&PebData->InMemoryOrderModuleList, &Module->InMemoryOrderLinks);
    }
    else
    {
        InsertTailList(&PebData->InLoadOrderModuleList, &Module->InLoadOrderLinks);
        InsertTailList(&PebData->InMemoryOrderModuleList, &Module->InMemoryOrderLinks);
    }
    
    RtlpCheckListEntry(&PebData->InLoadOrderModuleList);
    RtlpCheckListEntry(&PebData->InMemoryOrderModuleList);

    Module->InLegacyLists = TRUE;
}
