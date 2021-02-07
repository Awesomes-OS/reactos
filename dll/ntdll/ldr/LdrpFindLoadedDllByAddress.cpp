#include <ldrp.h>

NTSTATUS
NTAPI
LdrpFindLoadedDllByAddress(IN PVOID Base,
                           OUT PLDR_DATA_TABLE_ENTRY *Module,
                           OUT LDR_DDAG_STATE *DdagState OPTIONAL)
{
    PLDR_DATA_TABLE_ENTRY FoundEntry = NULL;

    if (Base)
    {
        RtlAcquireSRWLockExclusive(&LdrpModuleDatatableLock);

        /* Check the cache first */
        if (LdrpLoadedDllHandleCache && LdrpLoadedDllHandleCache->DllBase == Base)
        {
            /* We got lucky, return the cached entry */
            FoundEntry = LdrpLoadedDllHandleCache;
            goto Quickie;
        }

        /* Time for a lookup */
        {
            const auto* const ListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
            for (auto* Next = ListHead->Flink; Next != ListHead; Next = Next->Flink)
            {
                /* Get the current entry */
                auto* const Current = CONTAINING_RECORD(Next, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

                /* Make sure it's not unloading and check for a match */
                if (Current->InMemoryOrderLinks.Flink && Base == Current->DllBase)
                {
                    /* Save in cache */
                    LdrpLoadedDllHandleCache = Current;

                    /* Return it */
                    FoundEntry = Current;
                    goto Quickie;
                }
            }
        }

Quickie:
        if (FoundEntry && !LDRP_MODULE_PINNED(FoundEntry))
        {
            InterlockedIncrement(reinterpret_cast<long volatile*>(&FoundEntry->ReferenceCount));
        }

        RtlReleaseSRWLockExclusive(&LdrpModuleDatatableLock);
    }

    *Module = FoundEntry;
    if (FoundEntry && DdagState)
        *DdagState = FoundEntry->DdagNode->State;

    /* Nothing found */
    return FoundEntry ? STATUS_SUCCESS : STATUS_DLL_NOT_FOUND;
}
