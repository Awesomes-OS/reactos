#include <ldrp.h>
#include <reactos/ldrp.h>

NTSTATUS
NTAPI
LdrpProcessMappedModule(PLDR_DATA_TABLE_ENTRY Module, LDRP_LOAD_CONTEXT_FLAGS LoadContextFlags, BOOL AdvanceLoad)
{
    PIMAGE_NT_HEADERS NtHeaders;

    /* Get entry point offset from NT headers */
    NtHeaders = RtlImageNtHeader(Module->DllBase);

    if (Module->ImageDll && !Module->CorILOnly)
    {
        Module->EntryPoint = LdrpFetchAddressOfEntryPoint(Module->DllBase);
    }

    // todo: LdrpValidateEntrySection

    // todo: SWAPD/SWAPQ from rtl.h to support other endianess
    if (NtHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        Module->OriginalBase = ((PIMAGE_NT_HEADERS32)NtHeaders)->OptionalHeader.ImageBase;
    else if (NtHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
        Module->OriginalBase = ((PIMAGE_NT_HEADERS64)NtHeaders)->OptionalHeader.ImageBase;

    if (AdvanceLoad && Module->ImageDll && !Module->LoadConfigProcessed)
    {
        PIMAGE_LOAD_CONFIG_DIRECTORY LoadConfig = NULL;

        /* Setup the Cookie for the DLL */
        BOOLEAN CookieSet = LdrpInitSecurityCookie(Module->DllBase, Module->SizeOfImage,  NULL, 0, &LoadConfig);

        if (!CookieSet && Module->EntryPoint)
        {
            // Windows 8.1+ PE requirement
            if (NtHeaders->OptionalHeader.MajorSubsystemVersion > 6 || (NtHeaders->OptionalHeader.MajorSubsystemVersion == 6 && NtHeaders->OptionalHeader.MinorSubsystemVersion >= 3))
            {
                return STATUS_INVALID_IMAGE_FORMAT;
            }
        }

        // FIXME: Load Config not implemented
    }

    if (!Module->InExceptionTable)
    {
        // todo: RtlInsertInvertedFunctionTable(Module->DllBase, Module->SizeOfImage)
    }

    Module->LoadConfigProcessed = TRUE;
    Module->InExceptionTable = TRUE;

    RtlAcquireSRWLockExclusive(&LdrpModuleDatatableLock);

    Module->DdagNode->State = LdrModulesMapped;

    if (Module->LoadContext)
        LdrpSignalModuleMapped(Module);

    RtlReleaseSRWLockExclusive(&LdrpModuleDatatableLock);

    return STATUS_SUCCESS;
}
