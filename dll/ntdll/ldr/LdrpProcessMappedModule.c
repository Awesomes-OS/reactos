#include <ldrp.h>
#include <reactos/ldrp.h>

NTSTATUS
NTAPI
LdrpProcessMappedModule(PLDR_DATA_TABLE_ENTRY LdrEntry, LDRP_LOAD_CONTEXT_FLAGS LoadContextFlags, BOOL AdvanceLoad)
{
    PIMAGE_NT_HEADERS NtHeaders;

    /* Get entry point offset from NT headers */
    NtHeaders = RtlImageNtHeader(LdrEntry->DllBase);

    if (LdrEntry->ImageDll && !LdrEntry->CorILOnly)
    {
        LdrEntry->EntryPoint = LdrpFetchAddressOfEntryPoint(LdrEntry->DllBase);
    }

    // todo: LdrpValidateEntrySection

    // todo: SWAPD/SWAPQ from rtl.h to support other endianess
    if (NtHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        LdrEntry->OriginalBase = ((PIMAGE_NT_HEADERS32)NtHeaders)->OptionalHeader.ImageBase;
    else if (NtHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
        LdrEntry->OriginalBase = ((PIMAGE_NT_HEADERS64)NtHeaders)->OptionalHeader.ImageBase;

    if (AdvanceLoad && LdrEntry->ImageDll && !LdrEntry->LoadConfigProcessed)
    {
        PIMAGE_LOAD_CONFIG_DIRECTORY LoadConfig = NULL;

        /* Setup the Cookie for the DLL */
        BOOLEAN CookieSet = LdrpInitSecurityCookie(LdrEntry->DllBase, LdrEntry->SizeOfImage,  NULL, 0, &LoadConfig);

        if (!CookieSet && LdrEntry->EntryPoint)
        {
            // Windows 8.1+ PE requirement
            if (NtHeaders->OptionalHeader.MajorSubsystemVersion > 6 || (NtHeaders->OptionalHeader.MajorSubsystemVersion == 6 && NtHeaders->OptionalHeader.MinorSubsystemVersion >= 3))
            {
                return STATUS_INVALID_IMAGE_FORMAT;
            }
        }

        // FIXME: Load Config not implemented
    }

    if (!LdrEntry->InExceptionTable)
    {
        // todo: RtlInsertInvertedFunctionTable(LdrEntry->DllBase, LdrEntry->SizeOfImage)
    }

    LdrEntry->LoadConfigProcessed = TRUE;
    LdrEntry->InExceptionTable = TRUE;

    RtlAcquireSRWLockExclusive(&LdrpModuleDatatableLock);

    LdrEntry->DdagNode->State = LdrModulesMapped;

    if (LdrEntry->LoadContext)
        LdrpSignalModuleMapped(LdrEntry);

    RtlReleaseSRWLockExclusive(&LdrpModuleDatatableLock);

    return STATUS_SUCCESS;
}
