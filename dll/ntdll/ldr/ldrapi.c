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
#include <ndk/mmfuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/iofuncs.h>
#include <include/ldrp.h>

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
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
LdrDestroyOutOfProcessImage(IN PVOID Image)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
LdrCreateOutOfProcessImage(IN ULONG Flags,
                           IN HANDLE ProcessHandle,
                           IN HANDLE DllHandle,
                           IN PVOID Unknown3)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
LdrAccessOutOfProcessResource(IN PVOID Unknown,
                              IN PVOID Image,
                              IN PVOID Unknown1,
                              IN PVOID Unknown2,
                              IN PVOID Unknown3)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

VOID
NTAPI
LdrSetDllManifestProber(
    _In_ PLDR_MANIFEST_PROBER_ROUTINE Routine)
{
    LdrpManifestProberRoutine = Routine;
}

BOOLEAN
NTAPI
LdrAlternateResourcesEnabled(VOID)
{
    /* ReactOS does not support this */
    return FALSE;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
DECLSPEC_HOTPATCH
LdrLoadDll(IN PWSTR SearchPath OPTIONAL,
           IN PULONG DllCharacteristics OPTIONAL,
           IN PUNICODE_STRING DllName,
           OUT PVOID* BaseAddress)
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PUNICODE_STRING OldTldDll;
    PTEB Teb = NtCurrentTeb();
    LDRP_LOAD_CONTEXT_FLAGS LoaderFlags = {0};

    if (DllCharacteristics)
        LoaderFlags = LdrpDllCharacteristicsToLoadFlags(*DllCharacteristics);

    LoaderFlags.CallInit = TRUE;

    /* Check if there's a TLD DLL being loaded */
    OldTldDll = LdrpTopLevelDllBeingLoaded;
    _SEH2_TRY
    {
        PLDR_DATA_TABLE_ENTRY LdrEntry = NULL;
        if (OldTldDll)
        {
            /* This is a recursive load, do something about it? */
            if ((ShowSnaps) || (LdrpShowRecursiveLoads) || (LdrpBreakOnRecursiveDllLoads))
            {
                /* Print out debug messages */
                DPRINT1("[%p, %p] LDR: Recursive DLL Load\n",
                        Teb->RealClientId.UniqueProcess,
                        Teb->RealClientId.UniqueThread);
                DPRINT1("[%p, %p]      Previous DLL being loaded \"%wZ\"\n",
                        Teb->RealClientId.UniqueProcess,
                        Teb->RealClientId.UniqueThread,
                        OldTldDll);
                DPRINT1("[%p, %p]      DLL being requested \"%wZ\"\n",
                        Teb->RealClientId.UniqueProcess,
                        Teb->RealClientId.UniqueThread,
                        DllName);

                /* Was it initializing too? */
                if (!LdrpCurrentDllInitializer)
                {
                    DPRINT1("[%p, %p] LDR: No DLL Initializer was running\n",
                            Teb->RealClientId.UniqueProcess,
                            Teb->RealClientId.UniqueThread);
                }
                else
                {
                    DPRINT1("[%p, %p]      DLL whose initializer was currently running \"%wZ\"\n",
                            Teb->ClientId.UniqueProcess,
                            Teb->ClientId.UniqueThread,
                            &LdrpCurrentDllInitializer->BaseDllName);
                }
            }
        }

        /* Set this one as the TLD DLL being loaded*/
        LdrpTopLevelDllBeingLoaded = DllName;

        LDRP_PATH_SEARCH_CONTEXT PathSearchContext = {0};
        RtlInitUnicodeString(&PathSearchContext.DllSearchPath, SearchPath);

        /* Load the DLL */
        Status = LdrpLoadDll(DllName,
                             &PathSearchContext,
                             LoaderFlags,
                             &LdrEntry);

        if (NT_SUCCESS(Status))
        {
            ASSERT(BaseAddress);

            *BaseAddress = LdrEntry->DllBase;

            LdrpDereferenceModule(LdrEntry);
        }
    }
    _SEH2_FINALLY
    {
        /* Restore the old TLD DLL */
        LdrpTopLevelDllBeingLoaded = OldTldDll;
    }
    _SEH2_END;

    if (Status != STATUS_NO_SUCH_FILE &&
        Status != STATUS_DLL_NOT_FOUND &&
        Status != STATUS_OBJECT_NAME_NOT_FOUND &&
        Status != STATUS_DLL_INIT_FAILED)
    {
        DPRINT1("LDR: %s(%wZ) -> 0x%08lX\n",
            __FUNCTION__,
            DllName,
            Status);
    }

    /* Return */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrFindEntryForAddress(PVOID Address,
                       PLDR_DATA_TABLE_ENTRY *Module)
{
    PLIST_ENTRY ListHead, NextEntry;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    PIMAGE_NT_HEADERS NtHeader;
    PPEB_LDR_DATA Ldr = NtCurrentPeb()->Ldr;
    ULONG_PTR DllBase, DllEnd;

    DPRINT("LDR: %s(%p, %p)\n", __FUNCTION__, Address, Module);

    /* Nothing to do */
    if (!Ldr)
        return STATUS_NO_MORE_ENTRIES;

    /* Get the current entry */
    LdrEntry = Ldr->EntryInProgress;
    if (LdrEntry)
    {
        /* Get the NT Headers */
        NtHeader = RtlImageNtHeader(LdrEntry->DllBase);
        if (NtHeader)
        {
            /* Get the Image Base */
            DllBase = (ULONG_PTR) LdrEntry->DllBase;
            DllEnd = DllBase + NtHeader->OptionalHeader.SizeOfImage;

            /* Check if they match */
            if (((ULONG_PTR) Address >= DllBase) &&
                ((ULONG_PTR) Address < DllEnd))
            {
                /* Return it */
                *Module = LdrEntry;

                DPRINT1("LDR: %s(...) -> STATUS_SUCCESS [\"%wZ\"]\n", __FUNCTION__, &LdrEntry->BaseDllName);
                return STATUS_SUCCESS;
            }
        }
    }

    /* Loop the module list */
    ListHead = &Ldr->InMemoryOrderModuleList;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        /* Get the entry and NT Headers */
        LdrEntry = CONTAINING_RECORD(NextEntry, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
        NtHeader = RtlImageNtHeader(LdrEntry->DllBase);
        if (NtHeader)
        {
            /* Get the Image Base */
            DllBase = (ULONG_PTR) LdrEntry->DllBase;
            DllEnd = DllBase + NtHeader->OptionalHeader.SizeOfImage;

            /* Check if they match */
            if (((ULONG_PTR) Address >= DllBase) &&
                ((ULONG_PTR) Address < DllEnd))
            {
                /* Return it */
                *Module = LdrEntry;

                DPRINT1("LDR: %s(...) -> STATUS_SUCCESS [\"%wZ\"]\n", __FUNCTION__, &LdrEntry->BaseDllName);
                return STATUS_SUCCESS;
            }

            /* Next Entry */
            NextEntry = NextEntry->Flink;
        }
    }

    /* Nothing found */
    DPRINT1("LDR: %s(...) -> STATUS_NO_MORE_ENTRIES\n", __FUNCTION__);
    return STATUS_NO_MORE_ENTRIES;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrGetProcedureAddress(IN PVOID BaseAddress,
                       IN PANSI_STRING Name,
                       IN ULONG Ordinal,
                       OUT PVOID *ProcedureAddress)
{
    return LdrGetProcedureAddressForCaller(BaseAddress,
                                           Name,
                                           Ordinal,
                                           ProcedureAddress,
                                           0,
                                           _ReturnAddress());
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrGetProcedureAddressEx(IN PVOID BaseAddress,
                         IN PANSI_STRING FunctionName OPTIONAL,
                         IN ULONG Ordinal OPTIONAL,
                         OUT PVOID *ProcedureAddress,
                         IN UINT8 Flags)
{
    return LdrGetProcedureAddressForCaller(BaseAddress,
                                           FunctionName,
                                           Ordinal,
                                           ProcedureAddress,
                                           Flags,
                                           _ReturnAddress());
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
    FILE_STANDARD_INFORMATION FileStandardInfo;
    PIMAGE_IMPORT_DESCRIPTOR ImportData;
    PIMAGE_SECTION_HEADER LastSection = NULL;
    IO_STATUS_BLOCK IoStatusBlock;
    PIMAGE_NT_HEADERS NtHeader;
    HANDLE SectionHandle;
    SIZE_T ViewSize;
    PVOID ViewBase;
    BOOLEAN Result, NoActualCheck;
    NTSTATUS Status;
    PVOID ImportName;
    ULONG Size;
    DPRINT("LdrVerifyImageMatchesChecksum() called\n");

    /* If the handle has the magic KnownDll flag, skip actual checksums */
    NoActualCheck = ((ULONG_PTR) FileHandle & 1);

    /* Create the section */
    Status = NtCreateSection(&SectionHandle,
                             SECTION_MAP_EXECUTE,
                             NULL,
                             NULL,
                             PAGE_EXECUTE,
                             SEC_COMMIT,
                             FileHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1 ("NtCreateSection() failed (Status 0x%x)\n", Status);
        return Status;
    }

    /* Map the section */
    ViewSize = 0;
    ViewBase = NULL;
    Status = NtMapViewOfSection(SectionHandle,
                                NtCurrentProcess(),
                                &ViewBase,
                                0,
                                0,
                                NULL,
                                &ViewSize,
                                ViewShare,
                                0,
                                PAGE_EXECUTE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtMapViewOfSection() failed (Status 0x%08lX)\n", Status);
        NtClose(SectionHandle);
        return Status;
    }

    /* Get the file information */
    Status = NtQueryInformationFile(FileHandle,
                                    &IoStatusBlock,
                                    &FileStandardInfo,
                                    sizeof(FILE_STANDARD_INFORMATION),
                                    FileStandardInformation);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtMapViewOfSection() failed (Status 0x%08lX)\n", Status);
        NtUnmapViewOfSection(NtCurrentProcess(), ViewBase);
        NtClose(SectionHandle);
        return Status;
    }

    /* Protect with SEH */
    _SEH2_TRY
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
            Result = LdrVerifyMappedImageMatchesChecksum(ViewBase,
                                                         FileStandardInfo.EndOfFile.LowPart,
                                                         FileStandardInfo.EndOfFile.LowPart);
        }

        /* Check if a callback was supplied */
        if ((Result) && (Callback))
        {
            /* Get the NT Header */
            NtHeader = RtlImageNtHeader(ViewBase);

            /* Check if caller requested this back */
            if (ImageCharacteristics)
            {
                /* Return to caller */
                *ImageCharacteristics = NtHeader->FileHeader.Characteristics;
            }

            /* Get the Import Directory Data */
            ImportData = RtlImageDirectoryEntryToData(ViewBase,
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
                    ImportName = RtlImageRvaToVa(NtHeader,
                                                 ViewBase,
                                                 ImportData->Name,
                                                 &LastSection);

                    /* Notify the callback */
                    Callback(CallbackContext, ImportName);
                    ImportData++;
                }
            }
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Fail the request returning STATUS_IMAGE_CHECKSUM_MISMATCH */
        Result = FALSE;
    }
    _SEH2_END;

    /* Unmap file and close handle */
    NtUnmapViewOfSection(NtCurrentProcess(), ViewBase);
    NtClose(SectionHandle);

    /* Return status */
    return Result ? Status : STATUS_IMAGE_CHECKSUM_MISMATCH;
}

NTSTATUS
NTAPI
LdrQueryProcessModuleInformationEx(IN ULONG ProcessId,
                                   IN ULONG Reserved,
                                   OUT PLDR_PROCESS_MODULES ModuleInformation,
                                   IN ULONG Size,
                                   OUT PULONG ReturnedSize OPTIONAL)
{
    PLIST_ENTRY ModuleListHead, InitListHead;
    PLIST_ENTRY Entry, InitEntry;
    PLDR_DATA_TABLE_ENTRY Module, InitModule;
    PRTL_PROCESS_MODULE_INFORMATION ModulePtr = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG UsedSize = sizeof(ULONG);
    ANSI_STRING AnsiString;
    PCHAR p;

    DPRINT("LdrQueryProcessModuleInformation() called\n");

    /* Acquire loader lock */
    RtlEnterCriticalSection(NtCurrentPeb()->LoaderLock);

    _SEH2_TRY
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
        ModuleListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
        Entry = ModuleListHead->Flink;

        while (Entry != ModuleListHead)
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
                InitListHead = &NtCurrentPeb()->Ldr->InInitializationOrderModuleList;
                InitEntry = InitListHead->Flink;

                while (InitEntry != InitListHead)
                {
                    InitModule = CONTAINING_RECORD(InitEntry, LDR_DATA_TABLE_ENTRY, InInitializationOrderLinks);

                    /* Increase the index */
                    ModulePtr->InitOrderIndex++;

                    /* Quit the loop if our module is found */
                    if (InitModule == Module)
                        break;

                    /* Advance to the next entry */
                    InitEntry = InitEntry->Flink;
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

            /* Go to the next entry in the modules list */
            Entry = Entry->Flink;
        }

        /* Set returned size if it was provided */
        if (ReturnedSize)
            *ReturnedSize = UsedSize;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Ignoring the exception */
    } _SEH2_END;

    /* Release the lock */
    RtlLeaveCriticalSection(NtCurrentPeb()->LoaderLock);

    DPRINT("LdrQueryProcessModuleInformation() done\n");

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
    PLIST_ENTRY ListHead, ListEntry;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    NTSTATUS Status;
    ULONG_PTR Cookie;
    BOOLEAN Stop = FALSE;

    /* Check parameters */
    if ((ReservedFlag) || !(EnumProc))
        return STATUS_INVALID_PARAMETER;

    /* Acquire the loader lock */
    Status = LdrLockLoaderLock(0, NULL, &Cookie);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Loop all the modules and call enum proc */
    ListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
    ListEntry = ListHead->Flink;
    while (ListHead != ListEntry)
    {
        /* Get the entry */
        LdrEntry = CONTAINING_RECORD(ListEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

        /* Call the enumeration proc inside SEH */
        _SEH2_TRY
        {
            EnumProc(LdrEntry, Context, &Stop);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Ignoring the exception */
        } _SEH2_END;

        /* Break if we were asked to stop enumeration */
        if (Stop)
        {
            /* Release loader lock */
            Status = LdrUnlockLoaderLock(0, Cookie);

            /* Reset any successful status to STATUS_SUCCESS, but leave
               failure to the caller */
            if (NT_SUCCESS(Status))
                Status = STATUS_SUCCESS;

            /* Return any possible failure status */
            return Status;
        }

        /* Advance to the next module */
        ListEntry = ListEntry->Flink;
    }

    /* Release loader lock, it must succeed this time */
    Status = LdrUnlockLoaderLock(0, Cookie);
    ASSERT(NT_SUCCESS(Status));

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
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    LDR_DDAG_STATE DdagState;
    NTSTATUS Status;
    DPRINT("LdrDisableThreadCalloutsForDll (BaseAddress %p)\n", BaseAddress);

    /* Don't do it during shutdown */
    if (LdrpShutdownInProgress)
        return STATUS_SUCCESS;

    /* Make sure the DLL is valid and get its entry */
    if (NT_SUCCESS(Status = LdrpFindLoadedDllByHandle(BaseAddress, &LdrEntry, &DdagState)))
    {
        /* Get if it has a TLS slot */
        if (!LdrEntry->TlsIndex)
        {
            /* It doesn't, so you're allowed to call this */
            LdrEntry->DontCallForThreads = TRUE;
        }

        LdrpDereferenceModule(LdrEntry);
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
    PLDR_DATA_TABLE_ENTRY LdrEntry = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    LDR_DDAG_STATE DdagState;

    const BOOLEAN Pin = Flags & LDR_ADDREF_DLL_PIN;
    Flags &= ~LDR_ADDREF_DLL_PIN;

    /* Check for invalid flags */
    if (Flags)
        return STATUS_INVALID_PARAMETER;

    Status = LdrpFindLoadedDllByHandle(BaseAddress, &LdrEntry, &DdagState);

    /* Get this module's data table entry */
    if (NT_SUCCESS(Status) && LdrEntry)
    {
        if (Pin)
        {
            Status = LdrpPinModule(LdrEntry);
        }
        else
        {
            Status = LdrpIncrementModuleLoadCount(LdrEntry);
        }

        LdrpDereferenceModule(LdrEntry);
    }

    /* Check for error case */
    if (!NT_SUCCESS(Status))
    {
        /* Print debug information */
        if ((ShowSnaps) || ((Status != STATUS_NO_SUCH_FILE) &&
            (Status != STATUS_DLL_NOT_FOUND) &&
            (Status != STATUS_OBJECT_NAME_NOT_FOUND)))
        {
            DPRINT1("LDR: LdrAddRefDll(%p) 0x%08lX\n", BaseAddress, Status);
        }
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
    NTSTATUS Status = STATUS_SUCCESS;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    LDR_DDAG_STATE DdagState;

    /* Skip unload */
    if (LdrpShutdownInProgress)
    {
        return Status;
    }

    /* Make sure the DLL is valid and get its entry */
    Status = LdrpFindLoadedDllByHandle(BaseAddress, &LdrEntry, &DdagState);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    if (LdrpDecrementNodeLoadCountEx(LdrEntry, TRUE) == STATUS_RETRY)
    {
        LdrpDrainWorkQueue();
        LdrpDecrementNodeLoadCountEx(LdrEntry, FALSE);

        Status = STATUS_SUCCESS;
    }

    LdrpDereferenceModule(LdrEntry);

    return Status;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
RtlDllShutdownInProgress(VOID)
{
    /* Return the internal global */
    return LdrpShutdownInProgress;
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

/* FIXME: Add to ntstatus.mc */
#define STATUS_MUI_FILE_NOT_FOUND        ((NTSTATUS)0xC00B0001L)

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
