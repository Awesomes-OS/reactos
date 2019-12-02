#include <ldrp.h>

PIMAGE_LOAD_CONFIG_DIRECTORY
NTAPI
LdrImageDirectoryEntryToLoadConfig(PVOID DllBase)
{
    PIMAGE_NT_HEADERS NtHeader;

    NTSTATUS Status = RtlImageNtHeaderEx(RTL_IMAGE_NT_HEADER_EX_FLAG_NO_RANGE_CHECK,
                                         DllBase,
                                         0,
                                         &NtHeader);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("LDR: %s(%p): RtlImageNtHeaderEx -> 0x%08lX\n", __FUNCTION__, DllBase, Status);
        return NULL;
    }

    PIMAGE_LOAD_CONFIG_DIRECTORY LoadConfig;
    ULONG LoadConfigSize;

    Status = RtlImageDirectoryEntryToDataEx(DllBase,
                                            TRUE,
                                            IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG,
                                            &LoadConfigSize,
                                            &LoadConfig);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("LDR: %s(%p): RtlImageDirectoryEntryToDataEx -> 0x%08lX\n", __FUNCTION__, DllBase, Status);

        LoadConfig = NULL;
    }


    if (!LoadConfig || !LoadConfigSize || LoadConfig->Size != LoadConfigSize)
        return NULL;

    switch (NtHeader->FileHeader.Machine)
    {
        case IMAGE_FILE_MACHINE_I386:
#ifdef _X86_
            return LoadConfig;
#elif defined(_AMD64_)
            return NULL;
#else
#error Add architecture-specific DEFINE to the list!
#endif
        case IMAGE_FILE_MACHINE_AMD64:
#ifdef _AMD64_
            return LoadConfig;
#elif defined(_X86_)
            return NULL;
#else
#error Add architecture-specific DEFINE to the list!
#endif
        default:
            DPRINT1("LDR: %s(%p): Unsupported IMAGE_FILE_MACHINE_* field -> 0x%X\n",
                    __FUNCTION__, DllBase, NtHeader->FileHeader.Machine);
            return NULL;
    }
}
