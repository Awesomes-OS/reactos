#include <ldrp.h>

NTSTATUS
NTAPI
LdrpIncrementModuleLoadCount(IN PLDR_DATA_TABLE_ENTRY Module)
{
    NTSTATUS Status = STATUS_SUCCESS;

    RtlAcquireSRWLockExclusive(&LdrpModuleDatatableLock);

    const PLDR_DDAG_NODE Node = Module->DdagNode;

    ULONG32* LoadCount = &Node->LoadCount;
    if (*LoadCount != LDR_LOADCOUNT_MAX)
    {
        if (*LoadCount)
        {
            *LoadCount += 1;
        }
        else if (NtCurrentTeb()->LoadOwner)
        {
            ASSERT(Node->State == LdrModulesUnloading);
            ++Node->LoadWhileUnloadingCount;
        }
        else
        {
            Status = STATUS_DLL_NOT_FOUND;
        }
    }

    RtlReleaseSRWLockExclusive(&LdrpModuleDatatableLock);

    return Status;
}
