#include <ldrp.h>

NTSTATUS
NTAPI
LdrpFindLoadedDllByHandle(IN PVOID Base,
                          OUT PLDR_DATA_TABLE_ENTRY *LdrEntry,
                          LDR_DDAG_STATE *DdagState)
{
    if (Base == LdrpSystemDllBase)
    {
        /* We got lucky, return the cached entry */
        *LdrEntry = LdrpNtDllDataTableEntry;

        if (DdagState)
            *DdagState = LdrpNtDllDataTableEntry->DdagNode->State;

        return STATUS_SUCCESS;
    }
    
    return LdrpFindLoadedDllByAddress(Base, LdrEntry, DdagState);
}
