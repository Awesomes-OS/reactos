#include <ldrp.h>

NTSTATUS
NTAPI
LdrpFindExistingModule(IN PUNICODE_STRING BaseDllName OPTIONAL,
                       IN PUNICODE_STRING FullDllName OPTIONAL,
                       IN LDRP_LOAD_CONTEXT_FLAGS LoadContextFlags OPTIONAL,
                       IN ULONG32 BaseNameHashValue,
                       OUT PLDR_DATA_TABLE_ENTRY* LdrEntry)
{
    NTSTATUS Status;
    RtlAcquireSRWLockExclusive(&LdrpModuleDatatableLock);

    Status = LdrpFindLoadedDllByNameLockHeld(BaseDllName,
                                             LoadContextFlags.HasFullPath ? FullDllName : NULL,
                                             LoadContextFlags,
                                             BaseNameHashValue,
                                             LdrEntry);

    if (!Status)
    {
        Status = LdrpFindLoadedDllByNameLockHeld(NULL,
                                                 FullDllName,
                                                 LoadContextFlags,
                                                 BaseNameHashValue,
                                                 LdrEntry);
    }

    RtlReleaseSRWLockExclusive(&LdrpModuleDatatableLock);
    return Status;
}