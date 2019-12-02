#include <ldrp.h>
#include <mmfuncs.h>

NTSTATUS
NTAPI
LdrpFindLoadedDllByMappingLockHeld(PVOID ViewBase, PIMAGE_NT_HEADERS NtHeader, PLDR_DATA_TABLE_ENTRY* LdrEntry)
{
    PLIST_ENTRY ListHead, ListEntry;
    NTSTATUS Status = STATUS_DLL_NOT_FOUND;

    /* Go through the list of modules again */
    ListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
    ListEntry = ListHead->Flink;
    while (ListEntry != ListHead)
    {
        /* Get the current entry and advance to the next one */
        const PLDR_DATA_TABLE_ENTRY CurEntry = CONTAINING_RECORD(ListEntry,
                                                                 LDR_DATA_TABLE_ENTRY,
                                                                 InLoadOrderLinks);
        ListEntry = ListEntry->Flink;

        /* Check if it's in the process of being unloaded */
        if (!CurEntry->InMemoryOrderLinks.Flink)
            continue;

        /* The header is untrusted, use SEH */
        _SEH2_TRY
        {
            /* Check if timedate stamp and sizes match */
            if ((CurEntry->TimeDateStamp == NtHeader->FileHeader.TimeDateStamp) &&
                (CurEntry->SizeOfImage == NtHeader->OptionalHeader.SizeOfImage))
            {
                /* Time, date and size match. Let's compare their headers */
                const PIMAGE_NT_HEADERS NtHeader2 = RtlImageNtHeader(CurEntry->DllBase);
                if (RtlCompareMemory(NtHeader2, NtHeader, sizeof(IMAGE_NT_HEADERS)))
                {
                    /* Headers match too! Finally ask the kernel to compare mapped files */
                    const NTSTATUS MapStatus = ZwAreMappedFilesTheSame(CurEntry->DllBase, ViewBase);
                    if (NT_SUCCESS(MapStatus))
                    {
                        /* This is our entry! */
                        *LdrEntry = CurEntry;
                        Status = STATUS_SUCCESS;
                        _SEH2_YIELD(break;)
                    }
                }
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            _SEH2_YIELD(break;)
        }
        _SEH2_END;
    }

    return Status;
}


NTSTATUS
NTAPI
LdrpFindLoadedDllByMapping(PVOID ViewBase, PIMAGE_NT_HEADERS NtHeader, PLDR_DATA_TABLE_ENTRY* LdrEntry,
                           LDR_DDAG_STATE* DdagState)
{
    RtlAcquireSRWLockExclusive(&LdrpModuleDatatableLock);

    const NTSTATUS Status = LdrpFindLoadedDllByMappingLockHeld(ViewBase, NtHeader, LdrEntry);

    RtlReleaseSRWLockExclusive(&LdrpModuleDatatableLock);

    if (NT_SUCCESS(Status) && DdagState)
        *DdagState = (*LdrEntry)->DdagNode->State;

    return Status;
}
