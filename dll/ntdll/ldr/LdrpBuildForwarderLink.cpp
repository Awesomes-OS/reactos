#include <ldrp.h>

NTSTATUS
NTAPI
LdrpBuildForwarderLink(IN PLDR_DATA_TABLE_ENTRY Source OPTIONAL,
                       IN PLDR_DATA_TABLE_ENTRY Target)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (Source)
    {
        RtlAcquireSRWLockExclusive(&LdrpModuleDatatableLock);

        if (Source->DdagNode->LoadCount || NtCurrentTeb()->LoadOwner)
        {
            LdrpRecordModuleDependency(Source, Target, NULL, &Status);
        }
        else
        {
            Status = STATUS_DLL_NOT_FOUND;
        }

        RtlReleaseSRWLockExclusive(&LdrpModuleDatatableLock);
    }

    return Status;
}
