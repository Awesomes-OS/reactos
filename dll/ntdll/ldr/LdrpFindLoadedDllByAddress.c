#include <ldrp.h>

NTSTATUS
NTAPI
LdrpFindLoadedDllByAddress(IN PVOID Base,
                           OUT PLDR_DATA_TABLE_ENTRY *LdrEntry,
                           LDR_DDAG_STATE *DdagState)
{
    PLDR_DATA_TABLE_ENTRY FoundEntry = NULL;

    if (Base)
    {
        PLIST_ENTRY ListHead, Next;

        RtlAcquireSRWLockExclusive(&LdrpModuleDatatableLock);

        /* Check the cache first */
        if ((LdrpLoadedDllHandleCache) &&
            (LdrpLoadedDllHandleCache->DllBase == Base))
        {
            /* We got lucky, return the cached entry */
            FoundEntry = LdrpLoadedDllHandleCache;
            goto Quickie;
        }

        /* Time for a lookup */
        ListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
        for (Next = ListHead->Flink; Next != ListHead; Next = Next->Flink)
        {
            /* Get the current entry */
            const PLDR_DATA_TABLE_ENTRY Current = CONTAINING_RECORD(Next, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

            /* Make sure it's not unloading and check for a match */
            if ((Current->InMemoryOrderLinks.Flink) && (Base == Current->DllBase))
            {
                /* Save in cache */
                LdrpLoadedDllHandleCache = Current;

                /* Return it */
                FoundEntry = Current;
                goto Quickie;
            }
        }

Quickie:
        if (FoundEntry && FoundEntry->DdagNode->LoadCount != LDR_LOADCOUNT_MAX && !FoundEntry->ProcessStaticImport)
        {
            InterlockedIncrement((LONG *) &FoundEntry->ReferenceCount);
        }

        RtlReleaseSRWLockExclusive(&LdrpModuleDatatableLock);
    }

    *LdrEntry = FoundEntry;
    if (FoundEntry && DdagState)
        *DdagState = FoundEntry->DdagNode->State;

    /* Nothing found */
    return FoundEntry ? STATUS_SUCCESS : STATUS_DLL_NOT_FOUND;
}
