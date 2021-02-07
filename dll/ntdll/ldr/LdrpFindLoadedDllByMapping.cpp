#include <ldrp.h>

EXTERN_C
{

#include <ndk/mmfuncs.h>

}

typedef struct _LDRP_MODULE_MAPPING_INFO
{
    DWORD TimeDateStamp;
    DWORD SizeOfImage;
} LDRP_MODULE_MAPPING_INFO, *PLDRP_MODULE_MAPPING_INFO;

NTSTATUS
NTAPI
LdrpFindLoadedDllByMappingLockHeld(PVOID ViewBase, PIMAGE_NT_HEADERS NtHeader, PLDRP_MODULE_MAPPING_INFO MappingInfo, PLDR_DATA_TABLE_ENTRY* Module)
{
    NTSTATUS Status = STATUS_DLL_NOT_FOUND;

    /* Go through the list of modules again */
    const PLIST_ENTRY ListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
    for (PLIST_ENTRY ListEntry = ListHead->Flink; ListEntry != ListHead;)
    {
        /* Get the current entry and advance to the next one */
        const PLDR_DATA_TABLE_ENTRY CurEntry = CONTAINING_RECORD(ListEntry,
                                                                 LDR_DATA_TABLE_ENTRY,
                                                                 InLoadOrderLinks);
        ListEntry = ListEntry->Flink;

        /* Check if it's in the process of being unloaded */
        if (!CurEntry->InMemoryOrderLinks.Flink)
            continue;

        /* Check if timedate stamp and sizes match */
        if (CurEntry->TimeDateStamp != MappingInfo->TimeDateStamp)
            continue;

        if (CurEntry->SizeOfImage != MappingInfo->SizeOfImage)
            continue;

        /* The header is untrusted, use SEH */
        __try
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

                    if (!LDRP_MODULE_PINNED(CurEntry))
                    {
                        ASSERT(CurEntry->ReferenceCount > 0);
                        InterlockedIncrement((LONG*)&CurEntry->ReferenceCount);
                    }

                    *Module = CurEntry;
                    Status = STATUS_SUCCESS;
                    _SEH2_YIELD(break;)
                }
            }
        }
        __except(LdrpDebugExceptionFilter(GetExceptionCode(), GetExceptionInformation()))
        {
            _SEH2_YIELD(break;)
        }
    }

    return Status;
}


NTSTATUS
NTAPI
LdrpFindLoadedDllByMapping(PVOID ViewBase, PIMAGE_NT_HEADERS NtHeader, PLDR_DATA_TABLE_ENTRY* Module,
                           LDR_DDAG_STATE* DdagState)
{
    LDRP_MODULE_MAPPING_INFO MappingInfo;

    MappingInfo.TimeDateStamp = NtHeader->FileHeader.TimeDateStamp;
    MappingInfo.SizeOfImage = NtHeader->OptionalHeader.SizeOfImage;

    RtlAcquireSRWLockExclusive(&LdrpModuleDatatableLock);

    const NTSTATUS Status = LdrpFindLoadedDllByMappingLockHeld(ViewBase, NtHeader, &MappingInfo, Module);

    if (NT_SUCCESS(Status) && DdagState)
        *DdagState = (*Module)->DdagNode->State;

    RtlReleaseSRWLockExclusive(&LdrpModuleDatatableLock);

    return Status;
}
