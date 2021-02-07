#include <ldrp.h>

NTSTATUS
NTAPI
LdrpFindLoadedDllByHandle(IN PVOID Base,
                          OUT PLDR_DATA_TABLE_ENTRY *Module,
                          LDR_DDAG_STATE *DdagState)
{
    if (Base == LdrpSystemDllBase)
    {
        /* We got lucky, return the cached entry */
        *Module = LdrpNtDllDataTableEntry;

        if (DdagState)
            *DdagState = LdrpNtDllDataTableEntry->DdagNode->State;

        return STATUS_SUCCESS;
    }
    
    return LdrpFindLoadedDllByAddress(Base, Module, DdagState);
}
