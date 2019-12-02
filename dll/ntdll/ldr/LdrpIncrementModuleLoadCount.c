#include <ldrp.h>

NTSTATUS
NTAPI
LdrpIncrementModuleLoadCount(IN PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    NTSTATUS Status = STATUS_SUCCESS;

    RtlAcquireSRWLockExclusive(&LdrpModuleDatatableLock);

    ULONG32* LoadCount = &LdrEntry->DdagNode->LoadCount;
    if (*LoadCount != LDR_LOADCOUNT_MAX)
    {
        if (*LoadCount)
        {
            *LoadCount += 1;
        }
        else
        {
            Status = STATUS_DLL_NOT_FOUND;
        }
    }

    RtlReleaseSRWLockExclusive(&LdrpModuleDatatableLock);

    return Status;
}
