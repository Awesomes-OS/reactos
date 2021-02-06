/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS NT User Mode Library
 * FILE:            dll/ntdll/ldr/ldrapi.c
 * PURPOSE:         PE Loader Public APIs
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ldrp.h>

EXTERN_C
{

#include <ndk/mmfuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/iofuncs.h>

}

#include <wil/resource.h>

using unique_ntdll_handle = wil::unique_any<HANDLE, decltype(&::NtClose), ::NtClose>;

namespace
{
    inline void __stdcall CloseSectionView(_In_ PVOID ViewBase) WI_NOEXCEPT
    {
        // todo (andrew.boyarshin): assert and ignore
        NtUnmapViewOfSection(NtCurrentProcess(), ViewBase);
    }
}

using unique_ntdll_section_handle = wil::unique_any<PVOID, decltype(&::CloseSectionView), ::CloseSectionView>;

/* GLOBALS *******************************************************************/

BOOLEAN LdrpShowRecursiveLoads, LdrpBreakOnRecursiveDllLoads;
UNICODE_STRING LdrpDefaultExtension = RTL_CONSTANT_STRING(L".DLL");
ULONG AlternateResourceModuleCount;

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
LdrFindCreateProcessManifest(IN ULONG Flags,
                             IN PVOID Image,
                             IN PVOID IdPath,
                             IN ULONG IdPathLength,
                             IN PVOID OutDataEntry)
{
    LDR_FUNC(NTSTATUS, Flags, Image, IdPath, IdPathLength, OutDataEntry)
        UNIMPLEMENTED;
        return STATUS_NOT_IMPLEMENTED;
    LDR_FUNC_END_RETURN
}

NTSTATUS
NTAPI
LdrDestroyOutOfProcessImage(IN PVOID Image)
{
    LDR_FUNC(NTSTATUS, Image)
        UNIMPLEMENTED;
        return STATUS_NOT_IMPLEMENTED;
    LDR_FUNC_END_RETURN
}

NTSTATUS
NTAPI
LdrCreateOutOfProcessImage(IN ULONG Flags,
                           IN HANDLE ProcessHandle,
                           IN HANDLE DllHandle,
                           IN PVOID Unknown3)
{
    LDR_FUNC(NTSTATUS, Flags, ProcessHandle, DllHandle, Unknown3)
        UNIMPLEMENTED;
        return STATUS_NOT_IMPLEMENTED;
    LDR_FUNC_END_RETURN
}

NTSTATUS
NTAPI
LdrAccessOutOfProcessResource(IN PVOID Unknown,
                              IN PVOID Image,
                              IN PVOID Unknown1,
                              IN PVOID Unknown2,
                              IN PVOID Unknown3)
{
    LDR_FUNC(NTSTATUS, Unknown, Image, Unknown1, Unknown2, Unknown3)
        UNIMPLEMENTED;
        return STATUS_NOT_IMPLEMENTED;
    LDR_FUNC_END_RETURN
}

VOID
NTAPI
LdrSetDllManifestProber(
    _In_ PLDR_MANIFEST_PROBER_ROUTINE Routine)
{
    LDR_FUNC_VOID(Routine)
        LdrpManifestProberRoutine = Routine;
    LDR_FUNC_END_VOID
}

BOOLEAN
NTAPI
LdrAlternateResourcesEnabled(VOID)
{
    LDR_FUNC_MANUAL(BOOLEAN)
        /* ReactOS does not support this */
        return FALSE;
    LDR_FUNC_END_IMPL(return FALSE)
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrLoadDll(IN PWSTR SearchPath OPTIONAL,
           IN PULONG DllCharacteristics OPTIONAL,
           IN PUNICODE_STRING DllName,
           OUT PVOID* BaseAddress)
{
    LDR_FUNC(NTSTATUS, SearchPath, DllCharacteristics, DllName, BaseAddress)
        NTSTATUS Status = STATUS_UNSUCCESSFUL;
        PUNICODE_STRING OldTldDll;
        PTEB Teb = NtCurrentTeb();
        LDRP_LOAD_CONTEXT_FLAGS LoaderFlags = {0};

        if (DllCharacteristics)
            LoaderFlags = LdrpDllCharacteristicsToLoadFlags(*DllCharacteristics);

        LoaderFlags.CallInit = TRUE;

        if (NtCurrentTeb()->LoaderWorker)
        {
            Status = STATUS_INVALID_THREAD;
        }
        else
        {
            /* Check if there's a TLD DLL being loaded */
            OldTldDll = LdrpTopLevelDllBeingLoaded;
            __try
            {
                PLDR_DATA_TABLE_ENTRY Module = NULL;
                if (OldTldDll)
                {
                    /* This is a recursive load, do something about it? */
                    if (LdrpShowRecursiveLoads || LdrpBreakOnRecursiveDllLoads)
                    {
                        /* Print out debug messages */
                        LDR_INFO(
                            "[" LDR_HEXPTR_FMT ", " LDR_HEXPTR_FMT "] LDR: Recursive DLL Load",
                            Teb->RealClientId.UniqueProcess, Teb->RealClientId.UniqueThread
                        );
                        LDR_INFO(
                            "[" LDR_HEXPTR_FMT ", " LDR_HEXPTR_FMT "]      Previous DLL being loaded \"{}\"",
                            Teb->RealClientId.UniqueProcess, Teb->RealClientId.UniqueThread, OldTldDll
                        );
                        LDR_INFO(
                            "[" LDR_HEXPTR_FMT ", " LDR_HEXPTR_FMT "]      DLL being requested \"{}\"",
                            Teb->RealClientId.UniqueProcess, Teb->RealClientId.UniqueThread, DllName
                        );

                        /* Was it initializing too? */
                        if (!LdrpCurrentDllInitializer)
                        {
                            LDR_INFO(
                                "[" LDR_HEXPTR_FMT ", " LDR_HEXPTR_FMT "] LDR: No DLL Initializer was running",
                                Teb->RealClientId.UniqueProcess, Teb->RealClientId.UniqueThread
                            );
                        }
                        else
                        {
                            LDR_INFO(
                                "[" LDR_HEXPTR_FMT ", " LDR_HEXPTR_FMT
                                "]      DLL whose initializer was currently running \"{}\"",
                                Teb->ClientId.UniqueProcess, Teb->ClientId.UniqueThread,
                                &LdrpCurrentDllInitializer->BaseDllName
                            );
                        }
                    }
                }

                /* Set this one as the TLD DLL being loaded */
                LdrpTopLevelDllBeingLoaded = DllName;

                LDRP_PATH_SEARCH_CONTEXT PathSearchContext = {0};
                RtlInitUnicodeString(&PathSearchContext.DllSearchPath, SearchPath);

                /* Load the DLL */
                Status = LdrpLoadDll(DllName, &PathSearchContext, LoaderFlags, &Module);

                if (NT_SUCCESS(Status))
                {
                    ASSERT_TRUE(BaseAddress);
                    ASSERT_TRUE(Module);

                    *BaseAddress = Module->DllBase;

                    LdrpDereferenceModule(Module);
                }
            }
            __finally
            {
                /* Restore the old TLD DLL */
                LdrpTopLevelDllBeingLoaded = OldTldDll;
            }
        }

        if (Status != STATUS_NO_SUCH_FILE &&
            Status != STATUS_DLL_NOT_FOUND &&
            Status != STATUS_OBJECT_NAME_NOT_FOUND &&
            Status != STATUS_DLL_INIT_FAILED)
        {
        }

        /* Return */
        return Status;
    LDR_FUNC_END_RETURN
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrFindEntryForAddress(PVOID Address,
                       PLDR_DATA_TABLE_ENTRY* Module)
{
    LDR_FUNC(NTSTATUS, Address, Module)
        PLDR_DATA_TABLE_ENTRY LdrEntry;
        PIMAGE_NT_HEADERS NtHeader;
        PPEB_LDR_DATA Ldr = NtCurrentPeb()->Ldr;
        ULONG_PTR DllBase, DllEnd;

        /* Nothing to do */
        if (!Ldr)
            return STATUS_NO_MORE_ENTRIES;

        /* Get the current entry */
        LdrEntry = (PLDR_DATA_TABLE_ENTRY)Ldr->EntryInProgress;
        if (LdrEntry)
        {
            /* Get the NT Headers */
            NtHeader = RtlImageNtHeader(LdrEntry->DllBase);
            if (NtHeader)
            {
                /* Get the Image Base */
                DllBase = (ULONG_PTR)LdrEntry->DllBase;
                DllEnd = DllBase + NtHeader->OptionalHeader.SizeOfImage;

                /* Check if they match */
                if (((ULONG_PTR)Address >= DllBase) &&
                    ((ULONG_PTR)Address < DllEnd))
                {
                    /* Return it */
                    *Module = LdrEntry;

                    return STATUS_SUCCESS;
                }
            }
        }

        /* Loop the module list */
        const auto* const ListHead = &Ldr->InMemoryOrderModuleList;
        for (const auto* NextEntry = ListHead->Flink; NextEntry != ListHead; NextEntry = NextEntry->Flink)
        {
            /* Get the entry and NT Headers */
            LdrEntry = CONTAINING_RECORD(NextEntry, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
            NtHeader = RtlImageNtHeader(LdrEntry->DllBase);

            if (!NtHeader)
                continue;

            /* Get the Image Base */
            DllBase = (ULONG_PTR)LdrEntry->DllBase;
            DllEnd = DllBase + NtHeader->OptionalHeader.SizeOfImage;

            /* Check if they match */
            if (((ULONG_PTR)Address >= DllBase) &&
                ((ULONG_PTR)Address < DllEnd))
            {
                /* Return it */
                *Module = LdrEntry;
                return STATUS_SUCCESS;
            }
        }

        return STATUS_NO_MORE_ENTRIES;
    LDR_FUNC_END_RETURN
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrGetProcedureAddress(IN PVOID BaseAddress,
                       IN PANSI_STRING Name,
                       IN ULONG Ordinal,
                       OUT PVOID* ProcedureAddress)
{
    LDR_FUNC(NTSTATUS, BaseAddress, Name, Ordinal, ProcedureAddress)
        return LdrGetProcedureAddressForCaller(BaseAddress,
                                               Name,
                                               Ordinal,
                                               ProcedureAddress,
                                               0,
                                               (PVOID*)_ReturnAddress());
    LDR_FUNC_END_RETURN
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrGetProcedureAddressEx(IN PVOID BaseAddress,
                         IN PANSI_STRING FunctionName OPTIONAL,
                         IN ULONG Ordinal OPTIONAL,
                         OUT PVOID* ProcedureAddress,
                         IN UINT8 Flags)
{
    LDR_FUNC(NTSTATUS, BaseAddress, FunctionName, Ordinal, ProcedureAddress, Flags)
        return LdrGetProcedureAddressForCaller(BaseAddress,
                                               FunctionName,
                                               Ordinal,
                                               ProcedureAddress,
                                               Flags,
                                               (PVOID*)_ReturnAddress());
    LDR_FUNC_END_RETURN
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrVerifyImageMatchesChecksum(IN HANDLE FileHandle,
                              IN PLDR_CALLBACK Callback,
                              IN PVOID CallbackContext,
                              OUT PUSHORT ImageCharacteristics)
{
    LDR_FUNC(NTSTATUS, FileHandle, Callback, CallbackContext, ImageCharacteristics)
        FILE_STANDARD_INFORMATION FileStandardInfo;
        PIMAGE_SECTION_HEADER LastSection = NULL;
        IO_STATUS_BLOCK IoStatusBlock;
        SIZE_T ViewSize;
        BOOLEAN Result, NoActualCheck;
        NTSTATUS Status;
        ULONG Size;

        /* If the handle has the magic KnownDll flag, skip actual checksums */
        NoActualCheck = ((ULONG_PTR)FileHandle & 1);

        /* Create the section */
        unique_ntdll_handle SectionHandle;

        Status = NtCreateSection(SectionHandle.put(),
                                 SECTION_MAP_EXECUTE,
                                 NULL,
                                 NULL,
                                 PAGE_EXECUTE,
                                 SEC_COMMIT,
                                 FileHandle);

        ASSERT_SUCCESS_OR_RETURN(Status);

        /* Map the section */
        unique_ntdll_section_handle ViewBase;
        ViewSize = 0;
        Status = NtMapViewOfSection(SectionHandle.get(),
                                    NtCurrentProcess(),
                                    ViewBase.put(),
                                    0,
                                    0,
                                    NULL,
                                    &ViewSize,
                                    ViewShare,
                                    0,
                                    PAGE_EXECUTE);

        ASSERT_SUCCESS_OR_RETURN(Status);

        /* Get the file information */
        Status = NtQueryInformationFile(FileHandle,
                                        &IoStatusBlock,
                                        &FileStandardInfo,
                                        sizeof(FILE_STANDARD_INFORMATION),
                                        FileStandardInformation);

        ASSERT_SUCCESS_OR_RETURN(Status);

        /* Protect with SEH */
        __try
        {
            /* Check if this is the KnownDll hack */
            if (NoActualCheck)
            {
                /* Don't actually do it */
                Result = TRUE;
            }
            else
            {
                /* Verify the checksum */
                Result = LdrVerifyMappedImageMatchesChecksum(ViewBase.get(),
                                                             FileStandardInfo.EndOfFile.LowPart,
                                                             FileStandardInfo.EndOfFile.LowPart);
            }

            /* Check if a callback was supplied */
            if (Result && Callback)
            {
                /* Get the NT Header */
                auto* const NtHeader = RtlImageNtHeader(ViewBase.get());

                /* Check if caller requested this back */
                if (ImageCharacteristics)
                {
                    /* Return to caller */
                    *ImageCharacteristics = NtHeader->FileHeader.Characteristics;
                }

                /* Get the Import Directory Data */
                const auto* ImportData = (PIMAGE_IMPORT_DESCRIPTOR)RtlImageDirectoryEntryToData(ViewBase.get(),
                    FALSE,
                    IMAGE_DIRECTORY_ENTRY_IMPORT,
                    &Size);

                /* Make sure there is one */
                if (ImportData)
                {
                    /* Loop the imports */
                    while (ImportData->Name)
                    {
                        /* Get the name */
                        auto* const ImportName = RtlImageRvaToVa(NtHeader,
                                                                 ViewBase.get(),
                                                                 ImportData->Name,
                                                                 &LastSection);

                        /* Notify the callback */
                        Callback(CallbackContext, static_cast<PCHAR>(ImportName));
                        ImportData++;
                    }
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            /* Fail the request returning STATUS_IMAGE_CHECKSUM_MISMATCH */
            Result = FALSE;
        }

        /* Return status */
        return Result ? Status : STATUS_IMAGE_CHECKSUM_MISMATCH;
    LDR_FUNC_END_RETURN
}

NTSTATUS
NTAPI
LdrQueryProcessModuleInformationEx(IN ULONG ProcessId,
                                   IN ULONG Reserved,
                                   OUT PLDR_PROCESS_MODULES ModuleInformation,
                                   IN ULONG Size,
                                   OUT PULONG ReturnedSize OPTIONAL)
{
    PLDR_DATA_TABLE_ENTRY Module;
    PRTL_PROCESS_MODULE_INFORMATION ModulePtr = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG UsedSize = sizeof(ULONG);
    ANSI_STRING AnsiString;
    PCHAR p;

    /* Acquire loader lock */
    RtlEnterCriticalSection(NtCurrentPeb()->LoaderLock);

    __try
    {
        /* Check if we were given enough space */
        if (Size < UsedSize)
        {
            Status = STATUS_INFO_LENGTH_MISMATCH;
        }
        else
        {
            ModuleInformation->NumberOfModules = 0;
            ModulePtr = &ModuleInformation->Modules[0];
            Status = STATUS_SUCCESS;
        }

        /* Traverse the list of modules */
        const PLIST_ENTRY ModuleListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
        for (PLIST_ENTRY Entry = ModuleListHead->Flink; Entry != ModuleListHead; Entry = Entry->Flink)
        {
            Module = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

            DPRINT("  Module %wZ\n", &Module->FullDllName);

            /* Increase the used size */
            UsedSize += sizeof(RTL_PROCESS_MODULE_INFORMATION);

            if (UsedSize > Size)
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
            }
            else
            {
                ModulePtr->ImageBase = Module->DllBase;
                ModulePtr->ImageSize = Module->SizeOfImage;
                ModulePtr->Flags = Module->Flags;
                ModulePtr->LoadCount = Module->DdagNode->LoadCount;
                ModulePtr->MappedBase = NULL;
                ModulePtr->InitOrderIndex = 0;
                ModulePtr->LoadOrderIndex = ModuleInformation->NumberOfModules;

                /* Now get init order index by traversing init list */
                const PLIST_ENTRY InitListHead = &NtCurrentPeb()->Ldr->InInitializationOrderModuleList;
                for (PLIST_ENTRY InitEntry = InitListHead->Flink; InitEntry != InitListHead; InitEntry = InitEntry->Flink)
                {
                    const PLDR_DATA_TABLE_ENTRY InitModule =
                        CONTAINING_RECORD(InitEntry, LDR_DATA_TABLE_ENTRY, InInitializationOrderLinks);

                    /* Increase the index */
                    ModulePtr->InitOrderIndex++;

                    /* Quit the loop if our module is found */
                    if (InitModule == Module)
                        break;
                }

                /* Prepare ANSI string with the module's name */
                AnsiString.Length = 0;
                AnsiString.MaximumLength = sizeof(ModulePtr->FullPathName);
                AnsiString.Buffer = ModulePtr->FullPathName;
                RtlUnicodeStringToAnsiString(&AnsiString,
                                             &Module->FullDllName,
                                             FALSE);

                /* Calculate OffsetToFileName field */
                p = strrchr(ModulePtr->FullPathName, '\\');
                if (p != NULL)
                    ModulePtr->OffsetToFileName = p - ModulePtr->FullPathName + 1;
                else
                    ModulePtr->OffsetToFileName = 0;

                /* Advance to the next module in the output list */
                ModulePtr++;

                /* Increase number of modules */
                if (ModuleInformation)
                    ModuleInformation->NumberOfModules++;
            }
        }

        /* Set returned size if it was provided */
        if (ReturnedSize)
            *ReturnedSize = UsedSize;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Ignoring the exception */
    }
    /* Release the lock */
    RtlLeaveCriticalSection(NtCurrentPeb()->LoaderLock);

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrQueryProcessModuleInformation(IN PLDR_PROCESS_MODULES ModuleInformation,
                                 IN ULONG Size,
                                 OUT PULONG ReturnedSize OPTIONAL)
{
    /* Call Ex version of the API */
    return LdrQueryProcessModuleInformationEx(0, 0, ModuleInformation, Size, ReturnedSize);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrEnumerateLoadedModules(IN BOOLEAN ReservedFlag,
                          IN PLDR_ENUM_CALLBACK EnumProc,
                          IN PVOID Context)
{
    PLIST_ENTRY ListHead;
    NTSTATUS Status;
    ULONG_PTR Cookie;
    BOOLEAN LoadOwner;

    /* Check parameters */
    if (ReservedFlag || !EnumProc)
        return STATUS_INVALID_PARAMETER;

    LoadOwner = NtCurrentTeb()->LoadOwner;
    if (!LoadOwner)
        LdrpDrainWorkQueue(LoadComplete);

    /* Acquire the loader lock */
    Status = LdrLockLoaderLock(0, NULL, &Cookie);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Loop all the modules and call enum proc */
    ListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
    for (PLIST_ENTRY ListEntry = ListHead->Flink; ListHead != ListEntry; ListEntry = ListEntry->Flink)
    {
        BOOLEAN Stop = FALSE;

        /* Get the entry */
        PLDR_DATA_TABLE_ENTRY Module = CONTAINING_RECORD(ListEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

        /* Call the enumeration proc inside SEH */
        __try
        {
            EnumProc(Module, Context, &Stop);
        }
        __except(LdrpDebugExceptionFilter(GetExceptionCode(), GetExceptionInformation()))
        {
            /* Ignoring the exception */
        }

        /* Break if we were asked to stop enumeration */
        if (Stop)
            break;
    }

    /* Release loader lock, it must succeed this time */
    Status = LdrUnlockLoaderLock(0, Cookie);

    ASSERT_SUCCESS_OR_IGNORE(Status);

    if (!LoadOwner)
        LdrpDropLastInProgressCount();

    /* Return success */
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrDisableThreadCalloutsForDll(IN PVOID BaseAddress)
{
    PLDR_DATA_TABLE_ENTRY Module;
    LDR_DDAG_STATE DdagState;
    NTSTATUS Status;

    /* Don't do it during shutdown */
    if (NtCurrentPeb()->Ldr->ShutdownInProgress)
        return STATUS_SUCCESS;

    /* Make sure the DLL is valid and get its entry */
    if (NT_SUCCESS(Status = LdrpFindLoadedDllByHandle(BaseAddress, &Module, &DdagState)))
    {
        /* Get if it has a TLS slot */
        if (!Module->TlsIndex)
        {
            /* It doesn't, so you're allowed to call this */
            Module->DontCallForThreads = TRUE;
        }

        LdrpDereferenceModule(Module);
    }

    /* Return the status */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrAddRefDll(IN ULONG Flags,
             IN PVOID BaseAddress)
{
    PLDR_DATA_TABLE_ENTRY Module = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    LDR_DDAG_STATE DdagState;

    const BOOLEAN Pin = Flags & LDR_ADDREF_DLL_PIN;
    Flags &= ~LDR_ADDREF_DLL_PIN;

    /* Check for invalid flags */
    if (Flags)
        return STATUS_INVALID_PARAMETER;

    Status = LdrpFindLoadedDllByHandle(BaseAddress, &Module, &DdagState);

    /* Get this module's data table entry */
    if (NT_SUCCESS(Status) && Module)
    {
        if (Pin)
        {
            Status = LdrpPinModule(Module);
        }
        else
        {
            Status = LdrpIncrementModuleLoadCount(Module);
        }

        LdrpDereferenceModule(Module);
    }

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrUnloadDll(IN PVOID BaseAddress)
{
    const PTEB Teb = NtCurrentTeb();
    const PPEB Peb = Teb->ProcessEnvironmentBlock;
    NTSTATUS Status = STATUS_SUCCESS;
    PLDR_DATA_TABLE_ENTRY Module;
    LDR_DDAG_STATE DdagState;

    /* Skip unload */
    if (Peb->Ldr->ShutdownInProgress)
    {
        return Status;
    }

    /* Make sure the DLL is valid and get its entry */
    Status = LdrpFindLoadedDllByHandle(BaseAddress, &Module, &DdagState);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    if (LdrpDecrementModuleLoadCountEx(Module, TRUE) == STATUS_RETRY)
    {
        const BOOLEAN LoadOwner = Teb->LoadOwner;

        if (!LoadOwner)
            LdrpDrainWorkQueue(LoadComplete);

        LdrpDecrementModuleLoadCountEx(Module, FALSE);

        if (!LoadOwner)
            LdrpDropLastInProgressCount();

        Status = STATUS_SUCCESS;
    }

    LdrpDereferenceModule(Module);

    ASSERT_SUCCESS_OR_IGNORE(Status);

    return Status;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
RtlDllShutdownInProgress()
{
    /* Return the internal global */
    return NtCurrentPeb()->Ldr->ShutdownInProgress != FALSE;
}

/*
 * @implemented
 */
PIMAGE_BASE_RELOCATION
NTAPI
LdrProcessRelocationBlock(IN ULONG_PTR Address,
                          IN ULONG Count,
                          IN PUSHORT TypeOffset,
                          IN LONG_PTR Delta)
{
    return LdrProcessRelocationBlockLongLong(Address, Count, TypeOffset, Delta);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrLoadAlternateResourceModule(IN PVOID Module,
                               IN PWSTR Buffer)
{
    /* Is MUI Support enabled? */
    if (!LdrAlternateResourcesEnabled())
        return STATUS_SUCCESS;

    UNIMPLEMENTED;
    return STATUS_MUI_FILE_NOT_FOUND;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
LdrUnloadAlternateResourceModule(IN PVOID BaseAddress)
{
    ULONG_PTR Cookie;

    /* Acquire the loader lock */
    LdrLockLoaderLock(LDR_LOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS, NULL, &Cookie);

    /* Check if there's any alternate resources loaded */
    if (AlternateResourceModuleCount)
    {
        UNIMPLEMENTED;
    }

    /* Release the loader lock */
    LdrUnlockLoaderLock(1, Cookie);

    /* All done */
    return TRUE;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
LdrFlushAlternateResourceModules(VOID)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 * See https://www.kernelmode.info/forum/viewtopic.php?t=991
 */
NTSTATUS
NTAPI
LdrSetAppCompatDllRedirectionCallback(
    _In_ ULONG Flags,
    _In_ PLDR_APP_COMPAT_DLL_REDIRECTION_CALLBACK_FUNCTION CallbackFunction,
    _In_opt_ PVOID CallbackData)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
