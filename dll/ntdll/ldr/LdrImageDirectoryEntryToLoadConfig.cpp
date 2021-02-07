#include <ldrp.h>

PIMAGE_LOAD_CONFIG_DIRECTORY
NTAPI
LdrImageDirectoryEntryToLoadConfig(IN PVOID DllBase)
{
    LDR_FUNC(PIMAGE_LOAD_CONFIG_DIRECTORY, DllBase)
        PIMAGE_NT_HEADERS NtHeader;

        ASSERT_TRUE(DllBase);

        NTSTATUS Status = RtlImageNtHeaderEx(RTL_IMAGE_NT_HEADER_EX_FLAG_NO_RANGE_CHECK,
                                             DllBase,
                                             0,
                                             &NtHeader);

        ASSERT_SUCCESS(Status, return nullptr);

        PIMAGE_LOAD_CONFIG_DIRECTORY LoadConfig;
        ULONG LoadConfigSize;

        Status = RtlImageDirectoryEntryToDataEx(DllBase,
                                                TRUE,
                                                IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG,
                                                &LoadConfigSize,
                                                reinterpret_cast<PVOID*>(&LoadConfig));

        ASSERT_SUCCESS(Status, LoadConfig = nullptr);

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
                LDR_ERROR("Unsupported IMAGE_FILE_MACHINE_* field -> {}", NtHeader->FileHeader.Machine);
                return NULL;
        }
    LDR_FUNC_END_RETURN_NULL
}
