#include <ldrp.h>
#include <ndk/mmfuncs.h>

NTSTATUS
NTAPI
LdrpPrepareImportAddressTableForSnap(PLDRP_LOAD_CONTEXT LoadContext)
{
    NTSTATUS Status;
    PVOID Iat;

    // Get the IAT
    Status = RtlImageDirectoryEntryToDataEx(LoadContext->Entry->DllBase,
                                            TRUE,
                                            IMAGE_DIRECTORY_ENTRY_IAT,
                                            &LoadContext->IATSize,
                                            &Iat);

    if (!NT_SUCCESS(Status))
        Iat = NULL;

    LoadContext->IATBaseAddress = Iat;

    PIMAGE_NT_HEADERS NtHeader;

    Status = RtlImageNtHeaderEx(RTL_IMAGE_NT_HEADER_EX_FLAG_NO_RANGE_CHECK,
                                LoadContext->Entry->DllBase,
                                0,
                                &NtHeader);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("LDR: %s([\"%wZ\"]): RtlImageNtHeaderEx -> 0x%08lX",
                __FUNCTION__, &LoadContext->Entry->BaseDllName, Status);
        goto Quickie;
    }

    PIMAGE_LOAD_CONFIG_DIRECTORY LoadConfig = LdrImageDirectoryEntryToLoadConfig(LoadContext->Entry->DllBase);

    if (LoadConfig && LoadConfig->Size >= FIELD_OFFSET(IMAGE_LOAD_CONFIG_DIRECTORY, CodeIntegrity))
    {
        DPRINT1("LDR: %s([\"%wZ\"]): Windows 8.1/10 PE load config detected", __FUNCTION__,
                &LoadContext->Entry->BaseDllName);
        if (NtHeader->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_GUARD_CF)
        {
            if (LoadConfig->GuardFlags & IMAGE_GUARD_CF_INSTRUMENTED)
            {
                const ULONG_PTR CheckFunction = LoadConfig->GuardCFCheckFunctionPointer;
                LoadContext->GuardCFCheckFunctionPointerThunk = (PVOID)CheckFunction;
                if (CheckFunction)
                    LoadContext->GuardCFCheckFunctionPointer = *LoadContext->GuardCFCheckFunctionPointerThunk;
            }
        }
    }

    /* Check if we don't have one */
    if (!Iat)
    {
        PIMAGE_SECTION_HEADER SectionHeader = IMAGE_FIRST_SECTION(NtHeader);

        /* Get the RVA of the import directory */
        const ULONG Rva = NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;

        /* Make sure we got one */
        if (Rva)
        {
            /* Loop all the sections */
            for (ULONG i = 0; i < NtHeader->FileHeader.NumberOfSections; i++)
            {
                /* Check if we are inside this section */
                if ((Rva >= SectionHeader->VirtualAddress) &&
                    (Rva < (SectionHeader->VirtualAddress + SectionHeader->SizeOfRawData)))
                {
                    /* We are, so set the IAT here */
                    Iat = PTR_ADD_OFFSET(LoadContext->Entry->DllBase, SectionHeader->VirtualAddress);

                    LoadContext->IATBaseAddress = Iat;

                    /* Set the size */
                    LoadContext->IATSize = SectionHeader->Misc.VirtualSize;

                    /* Deal with Watcom and other retarded compilers */
                    if (!LoadContext->IATSize)
                        LoadContext->IATSize = SectionHeader->SizeOfRawData;

                    /* Found it, get out */
                    break;
                }

                /* No match, move to the next section */
                SectionHeader++;
            }
        }
    }

    if (LoadContext->IATBaseAddress && LoadContext->IATSize)
    {
        ULONG ImportSize = LoadContext->IATSize;

        /* Unprotect the IAT */
        Status = NtProtectVirtualMemory(NtCurrentProcess(),
                                        &Iat,
                                        &ImportSize,
                                        PAGE_READWRITE,
                                        &LoadContext->IATOriginalProtection);

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("LDR: %s([\"%wZ\"]): NtProtectVirtualMemory -> 0x%08lX",
                    __FUNCTION__, &LoadContext->Entry->BaseDllName, Status);
            return Status;
        }
    }

Quickie:

    return Status;
}
