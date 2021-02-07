/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS NT User-Mode Library
 * FILE:            dll/ntdll/ldr/ldrinit.c
 * PURPOSE:         User-Mode Process/Thread Startup
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ldrp.h>

EXTERN_C
{

#include <ndk/mmfuncs.h>
#include <ndk/cmfuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/psfuncs.h>
#include <ndk/kefuncs.h>
#include <ndk/exfuncs.h>
#include <psdk/winreg.h>
#include <compat_undoc.h>
#include <compatguid_undoc.h>

#include <ntdllp.h>
#include <stdio.h>

#include <reactos/ldrp.h>
#include <ndk/ntbits.h>

}

/* GLOBALS *******************************************************************/

HANDLE ImageExecOptionsKey;
HANDLE Wow64ExecOptionsKey;
const UNICODE_STRING ImageExecOptionsString = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options");
const UNICODE_STRING Wow64OptionsString = RTL_CONSTANT_STRING(L"");
const UNICODE_STRING NtDllString = RTL_CONSTANT_STRING(L"ntdll.dll");
const UNICODE_STRING LdrpKernel32DllName = RTL_CONSTANT_STRING(L"kernel32.dll");
const UNICODE_STRING LdrpDotLocal = RTL_CONSTANT_STRING(L".Local");

RTL_SRWLOCK LdrpModuleDatatableLock = RTL_SRWLOCK_INIT;

BOOLEAN UseWOW64 = FALSE;
LONG LdrpProcessInitialized;
ULONG LdrInitState;
PIMAGE_NT_HEADERS LdrpAppHeaders = NULL;
BOOLEAN LdrpDllValidation;
BOOLEAN LdrpNXProcessPermanent = FALSE;

PLDR_DATA_TABLE_ENTRY LdrpImageEntry;
PUNICODE_STRING LdrpTopLevelDllBeingLoaded;
WCHAR StringBuffer[156];
PLDR_DATA_TABLE_ENTRY LdrpCurrentDllInitializer;
PLDR_DATA_TABLE_ENTRY LdrpNtDllDataTableEntry;
PCONTEXT LdrpProcessInitContextRecord;

static NTSTATUS (WINAPI *Kernel32ProcessInitPostImportFunction)(void);
static BOOL (WINAPI *Kernel32BaseQueryModuleData)(IN LPSTR ModuleName, IN LPSTR Unk1, IN PVOID Unk2, IN PVOID Unk3, IN PVOID Unk4);

RTL_BITMAP TlsBitMap;
RTL_BITMAP TlsExpansionBitMap;
PVOID LdrpSystemDllBase;
LARGE_INTEGER RtlpTimeout;
BOOLEAN RtlpTimeoutDisable;
LIST_ENTRY LdrpHashTable[LDR_HASH_TABLE_ENTRIES];
LIST_ENTRY LdrpDllNotificationList;
HANDLE LdrpKnownDllDirectoryHandle;
UNICODE_STRING LdrpKnownDllPath;
WCHAR LdrpKnownDllPathBuffer[128];
UNICODE_STRING LdrpDefaultPath;

PEB_LDR_DATA PebLdr;

RTL_CRITICAL_SECTION_DEBUG LdrpLoaderLockDebug = {0};
RTL_CRITICAL_SECTION LdrpLoaderLock =
{
    &LdrpLoaderLockDebug,
    -1,
    0,
    0,
    0,
    0
};

RTL_CRITICAL_SECTION FastPebLock;
HANDLE LdrpInitCompleteEvent;

ULONG LdrpFatalHardErrorCount;

EXTERN_C LIST_ENTRY RtlCriticalSectionList;
EXTERN_C BOOLEAN RtlpCritSectInitialized;

EXTERN_C void NTAPI RtlpInitializeVectoredExceptionHandling(void);
EXTERN_C void NTAPI RtlpInitDeferredCriticalSection(void);
EXTERN_C void NTAPI RtlInitializeHeapManager(void);

ULONG RtlpDisableHeapLookaside; // TODO: Move to heap.c
ULONG RtlpShutdownProcessFlags; // TODO: Use it

EXTERN_C void actctx_init(PVOID* pOldShimData);
EXTERN_C BOOLEAN RtlpUse16ByteSLists;

#ifdef _WIN64
#define DEFAULT_SECURITY_COOKIE 0x00002B992DDFA232ll
#else
#define DEFAULT_SECURITY_COOKIE 0xBB40E64E
#define SECONDARY_DEFAULT_SECURITY_COOKIE 0xBB40
#endif

LONG SecurityCookieInitCount = 0;
BOOLEAN SecurityCookieInitialized = FALSE;

EXTERN_C ULONG32 SRWLockSpinCycleCount;

/* FUNCTIONS *****************************************************************/

EXTERN_C
void
RtlpInitializeKeyedEvent(void);

EXTERN_C
void
RtlpCloseKeyedEvent(void);

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrOpenImageFileOptionsKey(IN PUNICODE_STRING SubKey,
                           IN BOOLEAN Wow64,
                           OUT PHANDLE NewKeyHandle)
{
    LDR_FUNC(NTSTATUS, SubKey, Wow64, NewKeyHandle)
    PHANDLE RootKeyLocation;
    HANDLE RootKey;
    UNICODE_STRING SubKeyString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    PWCHAR p1;

    /* Check which root key to open */
    if (Wow64)
        RootKeyLocation = &Wow64ExecOptionsKey;
    else
        RootKeyLocation = &ImageExecOptionsKey;

    /* Get the current key */
    RootKey = *RootKeyLocation;

    /* Setup the object attributes */
    InitializeObjectAttributes(&ObjectAttributes,
                               const_cast<PUNICODE_STRING>(Wow64 ? &Wow64OptionsString : &ImageExecOptionsString),
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Open the root key */
    Status = ZwOpenKey(&RootKey, KEY_ENUMERATE_SUB_KEYS, &ObjectAttributes);
    if (NT_SUCCESS(Status))
    {
        /* Write the key handle */
        if (InterlockedCompareExchangePointer(RootKeyLocation, RootKey, NULL) != NULL)
        {
            /* Someone already opened it, use it instead */
            NtClose(RootKey);
            RootKey = *RootKeyLocation;
        }

        /* Extract the name */
        SubKeyString = *SubKey;
        p1 = (PWCHAR)((ULONG_PTR)SubKeyString.Buffer + SubKeyString.Length);
        while (SubKeyString.Length)
        {
            if (p1[-1] == L'\\') break;
            p1--;
            SubKeyString.Length -= sizeof(*p1);
        }
        SubKeyString.Buffer = p1;
        SubKeyString.Length = SubKey->Length - SubKeyString.Length;

        /* Setup the object attributes */
        InitializeObjectAttributes(&ObjectAttributes,
                                   &SubKeyString,
                                   OBJ_CASE_INSENSITIVE,
                                   RootKey,
                                   NULL);

        /* Open the setting key */
        Status = ZwOpenKey((PHANDLE)NewKeyHandle, GENERIC_READ, &ObjectAttributes);
    }

    /* Return to caller */
    return Status;
    LDR_FUNC_END_RETURN
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrQueryImageFileKeyOption(IN HANDLE KeyHandle,
                           IN PCWSTR ValueName,
                           IN ULONG Type,
                           OUT PVOID Buffer,
                           IN ULONG BufferSize,
                           OUT PULONG ReturnedLength OPTIONAL)
{
    LDR_FUNC(NTSTATUS, KeyHandle, ValueName, Type, Buffer, BufferSize, ReturnedLength)
    ULONG KeyInfo[256];
    UNICODE_STRING ValueNameString, IntegerString;
    ULONG KeyInfoSize, ResultSize;
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)&KeyInfo;
    BOOLEAN FreeHeap = FALSE;
    NTSTATUS Status;

    /* Build a string for the value name */
    Status = RtlInitUnicodeStringEx(&ValueNameString, ValueName);
    if (!NT_SUCCESS(Status)) return Status;

    /* Query the value */
    Status = ZwQueryValueKey(KeyHandle,
                             &ValueNameString,
                             KeyValuePartialInformation,
                             KeyValueInformation,
                             sizeof(KeyInfo),
                             &ResultSize);
    if (Status == STATUS_BUFFER_OVERFLOW)
    {
        /* Our local buffer wasn't enough, allocate one */
        KeyInfoSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION) +
                      KeyValueInformation->DataLength;
        KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION) RtlAllocateHeap(RtlGetProcessHeap(),
                                              0,
                                              KeyInfoSize);
        if (KeyValueInformation != NULL)
        {
            /* Try again */
            Status = ZwQueryValueKey(KeyHandle,
                                     &ValueNameString,
                                     KeyValuePartialInformation,
                                     KeyValueInformation,
                                     KeyInfoSize,
                                     &ResultSize);
            FreeHeap = TRUE;
        }
        else
        {
            /* Give up this time */
            Status = STATUS_NO_MEMORY;
        }
    }

    /* Check for success */
    if (NT_SUCCESS(Status))
    {
        /* Handle binary data */
        if (KeyValueInformation->Type == REG_BINARY)
        {
            /* Check validity */
            if ((Buffer) && (KeyValueInformation->DataLength <= BufferSize))
            {
                /* Copy into buffer */
                RtlMoveMemory(Buffer,
                              &KeyValueInformation->Data,
                              KeyValueInformation->DataLength);
            }
            else
            {
                Status = STATUS_BUFFER_OVERFLOW;
            }

            /* Copy the result length */
            if (ReturnedLength) *ReturnedLength = KeyValueInformation->DataLength;
        }
        else if (KeyValueInformation->Type == REG_DWORD)
        {
            /* Check for valid type */
            if (KeyValueInformation->Type != Type)
            {
                /* Error */
                Status = STATUS_OBJECT_TYPE_MISMATCH;
            }
            else
            {
                /* Check validity */
                if ((Buffer) &&
                    (BufferSize == sizeof(ULONG)) &&
                    (KeyValueInformation->DataLength <= BufferSize))
                {
                    /* Copy into buffer */
                    RtlMoveMemory(Buffer,
                                  &KeyValueInformation->Data,
                                  KeyValueInformation->DataLength);
                }
                else
                {
                    Status = STATUS_BUFFER_OVERFLOW;
                }

                /* Copy the result length */
                if (ReturnedLength) *ReturnedLength = KeyValueInformation->DataLength;
            }
        }
        else if (KeyValueInformation->Type != REG_SZ)
        {
            /* We got something weird */
            Status = STATUS_OBJECT_TYPE_MISMATCH;
        }
        else
        {
            /*  String, check what you requested */
            if (Type == REG_DWORD)
            {
                /* Validate */
                if (BufferSize != sizeof(ULONG))
                {
                    /* Invalid size */
                    BufferSize = 0;
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                }
                else
                {
                    /* OK, we know what you want... */
                    IntegerString.Buffer = (PWSTR)KeyValueInformation->Data;
                    IntegerString.Length = (USHORT)KeyValueInformation->DataLength -
                                           sizeof(WCHAR);
                    IntegerString.MaximumLength = (USHORT)KeyValueInformation->DataLength;
                    Status = RtlUnicodeStringToInteger(&IntegerString, 0, (PULONG)Buffer);
                }
            }
            else
            {
                /* Validate */
                if (KeyValueInformation->DataLength > BufferSize)
                {
                    /* Invalid */
                    Status = STATUS_BUFFER_OVERFLOW;
                }
                else
                {
                    /* Set the size */
                    BufferSize = KeyValueInformation->DataLength;
                }

                /* Copy the string */
                RtlMoveMemory(Buffer, &KeyValueInformation->Data, BufferSize);
            }

            /* Copy the result length */
            if (ReturnedLength) *ReturnedLength = KeyValueInformation->DataLength;
        }
    }

    /* Check if buffer was in heap */
    if (FreeHeap) RtlFreeHeap(RtlGetProcessHeap(), 0, KeyValueInformation);

    /* Return status */
    return Status;
    LDR_FUNC_END_RETURN
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrQueryImageFileExecutionOptionsEx(IN PUNICODE_STRING SubKey,
                                    IN PCWSTR ValueName,
                                    IN ULONG Type,
                                    OUT PVOID Buffer,
                                    IN ULONG BufferSize,
                                    OUT PULONG ReturnedLength OPTIONAL,
                                    IN BOOLEAN Wow64)
{
    LDR_FUNC(NTSTATUS, SubKey, ValueName, Type, Buffer, BufferSize, ReturnedLength, Wow64)
    NTSTATUS Status;
    HANDLE KeyHandle;

    /* Open a handle to the key */
    Status = LdrOpenImageFileOptionsKey(SubKey, Wow64, &KeyHandle);

    /* Check for success */
    if (NT_SUCCESS(Status))
    {
        /* Query the data */
        Status = LdrQueryImageFileKeyOption(KeyHandle,
                                            ValueName,
                                            Type,
                                            Buffer,
                                            BufferSize,
                                            ReturnedLength);

        /* Close the key */
        NtClose(KeyHandle);
    }

    /* Return to caller */
    return Status;
    LDR_FUNC_END_RETURN
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrQueryImageFileExecutionOptions(IN PUNICODE_STRING SubKey,
                                  IN PCWSTR ValueName,
                                  IN ULONG Type,
                                  OUT PVOID Buffer,
                                  IN ULONG BufferSize,
                                  OUT PULONG ReturnedLength OPTIONAL)
{
    LDR_FUNC(NTSTATUS, SubKey, ValueName, Type, Buffer, BufferSize, ReturnedLength)
    /* Call the newer function */
    return LdrQueryImageFileExecutionOptionsEx(SubKey,
                                               ValueName,
                                               Type,
                                               Buffer,
                                               BufferSize,
                                               ReturnedLength,
                                               FALSE);
    LDR_FUNC_END_RETURN
}

PULONG_PTR
NTAPI
LdrpFetchAddressOfSecurityCookie(PVOID BaseAddress,
                                 ULONG_PTR SizeOfImage,
                                 BOOLEAN* CookieNotWritable,
                                 OUT PIMAGE_LOAD_CONFIG_DIRECTORY* OutLoadConfig)
{
    LDR_FUNC(PULONG_PTR, BaseAddress, SizeOfImage, CookieNotWritable, OutLoadConfig)
    ASSERT_TRUE(CookieNotWritable);

    *CookieNotWritable = FALSE;

    if (OutLoadConfig)
        *OutLoadConfig = NULL;

    /* Check NT header first */
    PIMAGE_NT_HEADERS NtHeader;

    NTSTATUS Status = RtlImageNtHeaderEx(RTL_IMAGE_NT_HEADER_EX_FLAG_NO_RANGE_CHECK,
                                         BaseAddress,
                                         0,
                                         &NtHeader);

    ASSERT_SUCCESS(Status, return nullptr);

    PIMAGE_LOAD_CONFIG_DIRECTORY LoadConfig = LdrImageDirectoryEntryToLoadConfig(BaseAddress);

    if (!LoadConfig)
    {
        LDR_ERROR("LdrImageDirectoryEntryToLoadConfig -> NULL");
        return NULL;
    }

    if (LoadConfig->Size >= FIELD_OFFSET(IMAGE_LOAD_CONFIG_DIRECTORY, TimeDateStamp))
    {
        // We've got sane IMAGE_LOAD_CONFIG_DIRECTORY, return it if requested.
        if (OutLoadConfig)
            *OutLoadConfig = LoadConfig;
    }

    if (LoadConfig->Size < FIELD_OFFSET(IMAGE_LOAD_CONFIG_DIRECTORY, SEHandlerTable))
    {
        LDR_ERROR("Size ({}) is not sufficient to contain SecurityCookie at {}.",
                LoadConfig->Size,
                FIELD_OFFSET(IMAGE_LOAD_CONFIG_DIRECTORY, SecurityCookie));

        return NULL;
    }

    /* Now get the cookie */
    const ULONG_PTR Cookie = (ULONG_PTR)LoadConfig->SecurityCookie;
    const ULONG_PTR BaseAddressNum = (ULONG_PTR)BaseAddress;

    /* Check this cookie */
    if (Cookie <= BaseAddressNum ||
        Cookie >= BaseAddressNum + SizeOfImage - sizeof(PVOID))
    {
        LDR_ERROR("Cookie (" LDR_HEXPTR_FMT ") is outside image boundaries [" LDR_HEXPTR_FMT ".." LDR_HEXPTR_FMT "].",
                Cookie,
                BaseAddress,
                BaseAddressNum + SizeOfImage);

        return NULL;
    }

    PIMAGE_SECTION_HEADER SectionHeader = RtlImageRvaToSection(NtHeader, BaseAddress, Cookie - BaseAddressNum);

    if (SectionHeader && (SectionHeader->Characteristics & IMAGE_SCN_MEM_WRITE) == 0)
        *CookieNotWritable = TRUE;

    /* Return validated security cookie */
    return (PULONG_PTR)Cookie;
    LDR_FUNC_END_RETURN_NULL
}

ULONG_PTR LdrpGenSecurityCookie(ULONG_PTR StartCookie)
{
    LDR_FUNC(ULONG_PTR, StartCookie)
    ULONG_PTR NewCookie;
    LARGE_INTEGER Counter;

    /* Make up a cookie from a bunch of values which may uniquely represent
               current moment of time, environment, etc */

    NewCookie = (ULONG_PTR)NtCurrentTeb()->ClientId.UniqueProcess;
    NewCookie ^= (ULONG_PTR)NtCurrentTeb()->ClientId.UniqueThread;

    /* Loop like it's done in KeQueryTickCount(). We don't want to call it directly. */
    while (SharedUserData->SystemTime.High1Time != SharedUserData->SystemTime.High2Time)
    {
        YieldProcessor();
    }

    /* Calculate the milliseconds value and xor it to the cookie */

    const ULONG_PTR TicksPart1 = UInt32x32To64(SharedUserData->TickCountMultiplier, SharedUserData->TickCount.LowPart);
    const ULONG_PTR TicksPart2 = Int64ShrlMod32(TicksPart1, 24);
    NewCookie ^= TicksPart2 + (SharedUserData->TickCountMultiplier * (SharedUserData->TickCount.High1Time << 8));

    NtQueryPerformanceCounter(&Counter, NULL);

    return StartCookie ^ Counter.LowPart ^ Counter.HighPart ^ NewCookie;
    LDR_FUNC_END_IMPL(return 0)
}

BOOLEAN
NTAPI
LdrpInitSecurityCookie(PVOID BaseAddress,
                       ULONG_PTR SizeOfImage,
                       PULONG_PTR CookieOverride,
                       ULONG_PTR Salt,
                       OUT PIMAGE_LOAD_CONFIG_DIRECTORY* OutLoadConfig)
{
    LDR_FUNC(BOOLEAN, BaseAddress, SizeOfImage, CookieOverride, Salt, OutLoadConfig)
    PULONG_PTR Cookie;
    BOOLEAN CookieNotWritable;
    PIMAGE_LOAD_CONFIG_DIRECTORY LoadConfig = NULL;

    /* Fetch address of the cookie */
    if (CookieOverride)
    {
        Cookie = CookieOverride;
        CookieNotWritable = TRUE;
    }
    else
    {
        Cookie = LdrpFetchAddressOfSecurityCookie(BaseAddress, SizeOfImage, &CookieNotWritable, &LoadConfig);
    }

    if (OutLoadConfig)
        *OutLoadConfig = LoadConfig;

    if (LoadConfig && LoadConfig->Size >= FIELD_OFFSET(IMAGE_LOAD_CONFIG_DIRECTORY, CodeIntegrity))
    {
        LDR_DEBUG("Windows 8.1/10 PE load config detected");

        if (LoadConfig->GuardFlags & IMAGE_GUARD_SECURITY_COOKIE_UNUSED)
            return TRUE;
    }

    if (!Cookie)
        return FALSE;

    /* Check if it's a default one */
    if (*Cookie != DEFAULT_SECURITY_COOKIE)
    {
#ifdef SECONDARY_DEFAULT_SECURITY_COOKIE
        if (*Cookie != SECONDARY_DEFAULT_SECURITY_COOKIE)
        {
            return FALSE;
        }
#else
        return FALSE;
#endif
    }

    ULONG_PTR NewCookie = Salt ^ LdrpGenSecurityCookie((ULONG_PTR)Cookie);

    /* Make the cookie 16bit if necessary */
#ifdef SECONDARY_DEFAULT_SECURITY_COOKIE
    if (*Cookie == SECONDARY_DEFAULT_SECURITY_COOKIE)
        NewCookie &= 0xFFFF;
#endif

    // If the result is 0 or the same as we got, just add one to the existing value
    // and that's it
    if (NewCookie == 0)
#ifdef SECONDARY_DEFAULT_SECURITY_COOKIE
        NewCookie = SECONDARY_DEFAULT_SECURITY_COOKIE;
#else
        NewCookie = *Cookie;
#endif

    if (NewCookie == DEFAULT_SECURITY_COOKIE)
        ++NewCookie;

#ifdef SECONDARY_DEFAULT_SECURITY_COOKIE
    if (NewCookie == SECONDARY_DEFAULT_SECURITY_COOKIE)
        ++NewCookie;
#endif

    PVOID ProtectionAddress = Cookie;
    ULONG OldProtection = 0;
    SIZE_T ProtectionSize = 4;
    if (CookieNotWritable)
    {
        NTSTATUS Status = ZwProtectVirtualMemory(NtCurrentProcess(),
                                                 &ProtectionAddress,
                                                 &ProtectionSize,
                                                 PAGE_READWRITE,
                                                 &OldProtection);

        if (!NT_SUCCESS(Status))
            return FALSE;
    }

    // Set the new cookie value
    *Cookie = NewCookie;

    if (CookieNotWritable)
    {
        ZwProtectVirtualMemory(NtCurrentProcess(),
                               &ProtectionAddress,
                               &ProtectionSize,
                               OldProtection,
                               &OldProtection);
    }

    return TRUE;
    LDR_FUNC_END_IMPL(return FALSE)
}

void
NTAPI
InitSecurityCookie()
{
    LDR_FUNC_VOID_MANUAL()
    if (InterlockedIncrement(&SecurityCookieInitCount) == 1)
    {
        LdrpInitSecurityCookie(NULL, 0, (PULONG_PTR)&__security_cookie, 0, NULL);
        SecurityCookieInitialized = TRUE;
    }
    else
    {
        LARGE_INTEGER Delay = { .QuadPart = Int32x32To64(30, -10000) }; // 30ms
        while (!SecurityCookieInitialized)
            NtDelayExecution(FALSE, &Delay);
    }
    LDR_FUNC_END_VOID
}

void
NTAPI
LdrpInitializeThread(IN PCONTEXT Context)
{
    LDR_FUNC_VOID(Context)
    PTEB Teb = NtCurrentTeb();
    PPEB Peb = Teb->ProcessEnvironmentBlock;
    PLIST_ENTRY ListHead;
    RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_EXTENDED ActCtx;
    NTSTATUS Status;
    PVOID EntryPoint;

    LDR_DEBUG("LdrpInitializeThread() called for {} (" LDR_HEXPTR_FMT "/" LDR_HEXPTR_FMT ")",
            &LdrpImageEntry->BaseDllName,
            NtCurrentTeb()->RealClientId.UniqueProcess,
            NtCurrentTeb()->RealClientId.UniqueThread);

    if (UseCOR && Teb->InitialThread)
        LdrpCorReplaceStartContext(Context);

    /* Allocate an Activation Context Stack */
    LDR_DEBUG("ActivationContextStack " LDR_HEXPTR_FMT, NtCurrentTeb()->ActivationContextStackPointer);
    Status = RtlAllocateActivationContextStack(&NtCurrentTeb()->ActivationContextStackPointer);
    if (!NT_SUCCESS(Status))
    {
        LDR_WARNING("Unable to allocate ActivationContextStack");
    }

    /* Allocate TLS */
    while (TRUE)
    {
        Status = LdrpAllocateTls();
        if (Status != STATUS_NO_MEMORY)
            break;

        /* 300 milliseconds */
        LARGE_INTEGER Timeout = {
            .QuadPart = Int32x32To64(300, -10000)
        };

        /* Do the wait */
        Status = NtDelayExecution(FALSE, &Timeout);

        ASSERT_SUCCESS_OR_IGNORE(Status);
    }

    if (!NT_SUCCESS(Status))
    {
        NtTerminateProcess(NtCurrentProcess(), Status);
        RtlRaiseStatus(Status);
        return;
    }

    LdrpDrainWorkQueue(LoadComplete);

    /* Enter the Loader Lock */
    RtlEnterCriticalSection(&LdrpLoaderLock);

    ListHead = &Peb->Ldr->InMemoryOrderModuleList;
    for (PLIST_ENTRY NextEntry = ListHead->Flink; NextEntry != ListHead; NextEntry = NextEntry->Flink)
    {
        /* Get the current entry */
        PLDR_DATA_TABLE_ENTRY LdrEntry = CONTAINING_RECORD(NextEntry, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

        LDRP_ASSERT_MODULE_ENTRY(LdrEntry);

        /* Make sure it's not ourselves */
        if (Peb->ImageBaseAddress == LdrEntry->DllBase)
            continue;

        /* Check if we should call */
        if (LdrEntry->DontCallForThreads)
            continue;

        /* Get the entrypoint */
        EntryPoint = LdrEntry->EntryPoint;

        /* Check if we are ready to call it */
        if (!EntryPoint || !LdrEntry->ProcessAttachCalled || !LdrEntry->ImageDll)
            continue;

        /* Set up the Act Ctx */
        ActCtx.Size = sizeof(ActCtx);
        ActCtx.Format = 1;
        RtlZeroMemory(&ActCtx.Frame, sizeof(RTL_ACTIVATION_CONTEXT_STACK_FRAME));

        /* Activate the ActCtx */
        RtlActivateActivationContextUnsafeFast(&ActCtx,
            LdrEntry->EntryPointActivationContext);

        /* Make sure we're not shutting down */
        if (!Peb->Ldr->ShutdownInProgress)
        {
            __try
            {
                /* Check if it has TLS */
                if (LdrEntry->TlsIndex)
                {
                    /* Call TLS */
                    LdrpCallTlsInitializers(LdrEntry, DLL_THREAD_ATTACH);
                }

                /* Call the Entrypoint */
                LDR_TRACE("{} - Calling entry point at " LDR_HEXPTR_FMT " for thread attaching, " LDR_HEXPTR_FMT "/" LDR_HEXPTR_FMT,
                       &LdrEntry->BaseDllName, LdrEntry->EntryPoint,
                       NtCurrentTeb()->RealClientId.UniqueProcess,
                       NtCurrentTeb()->RealClientId.UniqueThread);
                LdrpCallInitRoutine(static_cast<PDLL_INIT_ROUTINE>(LdrEntry->EntryPoint),
                                    LdrEntry->DllBase,
                                    DLL_THREAD_ATTACH,
                                    NULL);
            }
            __except(LdrpDebugExceptionFilter(GetExceptionCode(), GetExceptionInformation()))
            {
                LDR_WARNING("Exception " LDR_HEX32_FMT " during LdrpCallInitRoutine(DLL_THREAD_ATTACH) for {}",
                        GetExceptionCode(), &LdrEntry->BaseDllName);
            }
        }

        /* Deactivate the ActCtx */
        RtlDeactivateActivationContextUnsafeFast(&ActCtx);
    }

    /* Check for TLS */
    if (LdrpImageEntry->TlsIndex && !Peb->Ldr->ShutdownInProgress)
    {
        /* Set up the Act Ctx */
        ActCtx.Size = sizeof(ActCtx);
        ActCtx.Format = 1;
        RtlZeroMemory(&ActCtx.Frame, sizeof(RTL_ACTIVATION_CONTEXT_STACK_FRAME));

        /* Activate the ActCtx */
        RtlActivateActivationContextUnsafeFast(&ActCtx,
                                               LdrpImageEntry->EntryPointActivationContext);

        []() -> void {
            __try
            {
                /* Do TLS callbacks */
                LdrpCallTlsInitializers(LdrpImageEntry, DLL_THREAD_ATTACH);
            }
            __except (LdrpDebugExceptionFilter(GetExceptionCode(), GetExceptionInformation()))
            {
                /* Do nothing */
            }
        }();

        /* Deactivate the ActCtx */
        RtlDeactivateActivationContextUnsafeFast(&ActCtx);
    }

    /* Release the lock */
    RtlLeaveCriticalSection(&LdrpLoaderLock);

    LdrpDropLastInProgressCount();

    LDR_FUNC_END_VOID
}

/*
 * @implemented
 */
void
NTAPI
LdrShutdownProcess()
{
    LDR_FUNC_VOID_MANUAL()
    PTEB Teb = NtCurrentTeb();
    PPEB Peb = Teb->ProcessEnvironmentBlock;
    PLIST_ENTRY ListHead;
    RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_EXTENDED ActCtx;
    PDLL_INIT_ROUTINE EntryPoint;

    if (Peb->Ldr->ShutdownInProgress)
        return;

    LDR_DEBUG("Process " LDR_HEXPTR_FMT " ({}) exiting", Teb->ClientId.UniqueProcess, &LdrpImageEntry->BaseDllName);

    /* Set the shutdown variables */
    Peb->Ldr->ShutdownThreadId = Teb->ClientId.UniqueThread;
    Peb->Ldr->ShutdownInProgress = TRUE;

    /* Tell the Shim Engine */
    if (g_ShimsEnabled)
    {
        void (NTAPI *SE_ProcessDying)() = static_cast<decltype(SE_ProcessDying)>(
            RtlDecodeSystemPointer(g_pfnSE_ProcessDying)
        );
        SE_ProcessDying();
    }

    /* Cleanup trace logging data (Etw) */
    if (SharedUserData->TraceLogging)
    {
        /* FIXME */
        LDR_INFO("We don't support Etw yet.");
    }

    ListHead = &Peb->Ldr->InInitializationOrderModuleList;

    /* Iterate backwards */
    for (PLIST_ENTRY NextEntry = ListHead->Blink; NextEntry != ListHead; NextEntry = NextEntry->Blink)
    {
        /* Get the current entry */
        PLDR_DATA_TABLE_ENTRY LdrEntry = CONTAINING_RECORD(NextEntry, LDR_DATA_TABLE_ENTRY, InInitializationOrderLinks);

        LDRP_ASSERT_MODULE_ENTRY(LdrEntry);

        /* Make sure it's not ourselves */
        if (Peb->ImageBaseAddress == LdrEntry->DllBase)
            continue;

        /* Get the entrypoint */
        EntryPoint = static_cast<PDLL_INIT_ROUTINE>(LdrEntry->EntryPoint);

        /* Check if we are ready to call it */
        if (!EntryPoint || !LdrEntry->ProcessAttachCalled)
            continue;

        /* Set up the Act Ctx */
        ActCtx.Size = sizeof(ActCtx);
        ActCtx.Format = 1;
        RtlZeroMemory(&ActCtx.Frame, sizeof(RTL_ACTIVATION_CONTEXT_STACK_FRAME));

        /* Activate the ActCtx */
        RtlActivateActivationContextUnsafeFast(&ActCtx,
                                               LdrEntry->EntryPointActivationContext);

        __try
        {
            /* Check if it has TLS */
            if (LdrEntry->TlsIndex)
            {
                /* Call TLS */
                LdrpCallTlsInitializers(LdrEntry, DLL_PROCESS_DETACH);
            }

            /* Call the Entrypoint */
            LDR_TRACE("{} - Calling entry point at " LDR_HEXPTR_FMT " for thread detaching",
                   &LdrEntry->BaseDllName, LdrEntry->EntryPoint);
            LdrpCallInitRoutine(EntryPoint,
                                LdrEntry->DllBase,
                                DLL_PROCESS_DETACH,
                                (PVOID)1);
        }
        __except(LdrpDebugExceptionFilter(GetExceptionCode(), GetExceptionInformation()))
        {
            DPRINT1("WARNING: Exception 0x%08lX during LdrpCallInitRoutine(DLL_PROCESS_DETACH) for %wZ\n",
                    GetExceptionCode(), &LdrEntry->BaseDllName);
        }

        /* Deactivate the ActCtx */
        RtlDeactivateActivationContextUnsafeFast(&ActCtx);
    }

    /* Check for TLS */
    if (LdrpImageEntry->TlsIndex)
    {
        /* Set up the Act Ctx */
        ActCtx.Size = sizeof(ActCtx);
        ActCtx.Format = 1;
        RtlZeroMemory(&ActCtx.Frame, sizeof(RTL_ACTIVATION_CONTEXT_STACK_FRAME));

        /* Activate the ActCtx */
        RtlActivateActivationContextUnsafeFast(&ActCtx,
                                               LdrpImageEntry->EntryPointActivationContext);

        __try
        {
            /* Do TLS callbacks */
            LdrpCallTlsInitializers(LdrpImageEntry, DLL_PROCESS_DETACH);
        }
        __except(LdrpDebugExceptionFilter(GetExceptionCode(), GetExceptionInformation()))
        {
            /* Do nothing */
        }
        /* Deactivate the ActCtx */
        RtlDeactivateActivationContextUnsafeFast(&ActCtx);
    }

    /* FIXME: Do Heap detection and Etw final shutdown */

    RtlpCloseKeyedEvent();
    LDR_FUNC_END_VOID
}

/*
 * @implemented
 */
void
NTAPI
LdrShutdownThread()
{
    LDR_FUNC_VOID_MANUAL()
    PTEB Teb = NtCurrentTeb();
    PPEB Peb = Teb->ProcessEnvironmentBlock;
    PLIST_ENTRY ListHead;
    RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_EXTENDED ActCtx;
    PDLL_INIT_ROUTINE EntryPoint;
    PVOID FlsData = Teb->FlsData;

    DPRINT("LdrShutdownThread() called for %wZ\n",
            &LdrpImageEntry->BaseDllName);

    /* Check for FLS Data */
    if (FlsData)
    {
        RtlProcessFlsData(FlsData, RTLP_FLS_DATA_CLEANUP_RUN_CALLBACKS);
    }

    /* Cleanup trace logging data (Etw) */
    if (SharedUserData->TraceLogging)
    {
        /* FIXME */
        DPRINT1("We don't support Etw yet.\n");
    }

    if (!Teb->LoaderWorker)
    {
        const BOOLEAN WasLoadOwner = Teb->LoadOwner;

        if (!WasLoadOwner)
            LdrpDrainWorkQueue(LoadComplete);

        /* Get the Ldr Lock */
        RtlEnterCriticalSection(&LdrpLoaderLock);

        ListHead = &Peb->Ldr->InInitializationOrderModuleList;

        /* Iterate backwards */
        for (PLIST_ENTRY NextEntry = ListHead->Blink; NextEntry != ListHead; NextEntry = NextEntry->Blink)
        {
            /* Get the current entry */
            PLDR_DATA_TABLE_ENTRY LdrEntry = CONTAINING_RECORD(NextEntry, LDR_DATA_TABLE_ENTRY, InInitializationOrderLinks);

            LDRP_ASSERT_MODULE_ENTRY(LdrEntry);

            /* Make sure it's not ourselves */
            if (Peb->ImageBaseAddress == LdrEntry->DllBase)
                continue;

            /* Check if we should call */
            if (LdrEntry->DontCallForThreads || !LdrEntry->ProcessAttachCalled || !LdrEntry->ImageDll)
                continue;

            /* Get the entrypoint */
            EntryPoint = static_cast<PDLL_INIT_ROUTINE>(LdrEntry->EntryPoint);

            /* Check if we are ready to call it */
            if (!EntryPoint)
                continue;

            /* Set up the Act Ctx */
            ActCtx.Size = sizeof(ActCtx);
            ActCtx.Format = 1;
            RtlZeroMemory(&ActCtx.Frame, sizeof(RTL_ACTIVATION_CONTEXT_STACK_FRAME));

            /* Activate the ActCtx */
            RtlActivateActivationContextUnsafeFast(&ActCtx, LdrEntry->EntryPointActivationContext);

            __try
            {
                /* Check if it has TLS */
                if (LdrEntry->TlsIndex)
                {
                    /* Call TLS */
                    LdrpCallTlsInitializers(LdrEntry, DLL_THREAD_DETACH);
                }

                /* Call the Entrypoint */
                DPRINT("%wZ - Calling entry point at " LDR_HEXPTR_FMT " for thread detaching\n",
                       &LdrEntry->BaseDllName, LdrEntry->EntryPoint);
                LdrpCallInitRoutine(EntryPoint, LdrEntry->DllBase, DLL_THREAD_DETACH, NULL);
            }
            __except(LdrpDebugExceptionFilter(GetExceptionCode(), GetExceptionInformation()))
            {
                DPRINT1("WARNING: Exception 0x%08lX during LdrpCallInitRoutine(DLL_THREAD_DETACH) for %wZ\n",
                        GetExceptionCode(), &LdrEntry->BaseDllName);
            }

            /* Deactivate the ActCtx */
            RtlDeactivateActivationContextUnsafeFast(&ActCtx);
        }

        /* Check for TLS */
        if (LdrpImageEntry->TlsIndex)
        {
            /* Set up the Act Ctx */
            ActCtx.Size = sizeof(ActCtx);
            ActCtx.Format = 1;
            RtlZeroMemory(&ActCtx.Frame, sizeof(RTL_ACTIVATION_CONTEXT_STACK_FRAME));

            /* Activate the ActCtx */
            RtlActivateActivationContextUnsafeFast(&ActCtx, LdrpImageEntry->EntryPointActivationContext);

            __try
            {
                /* Do TLS callbacks */
                LdrpCallTlsInitializers(LdrpImageEntry, DLL_THREAD_DETACH);
            }
            __except(LdrpDebugExceptionFilter(GetExceptionCode(), GetExceptionInformation()))
            {
                /* Do nothing */
            }

            /* Deactivate the ActCtx */
            RtlDeactivateActivationContextUnsafeFast(&ActCtx);
        }

        RtlLeaveCriticalSection(&LdrpLoaderLock);

        if (!WasLoadOwner)
            LdrpDropLastInProgressCount();

        /* Free TLS */
        LdrpFreeTls();
    }

    /* Check for expansion slots */
    if (Teb->TlsExpansionSlots)
    {
        /* Free expansion slots */
        RtlFreeHeap(RtlGetProcessHeap(), 0, Teb->TlsExpansionSlots);
    }

    /* Check for FLS Data */
    if (FlsData)
    {
        Teb->FlsData = NULL;
        RtlProcessFlsData(FlsData, RTLP_FLS_DATA_CLEANUP_DEALLOCATE);
    }

    /* Check for Fiber data */
    if (Teb->HasFiberData)
    {
        /* Free Fiber data*/
        RtlFreeHeap(RtlGetProcessHeap(), 0, Teb->NtTib.FiberData);
        Teb->NtTib.FiberData = NULL;
    }

    /* Free the activation context stack */
    RtlFreeThreadActivationContextStack();
    LDR_FUNC_END_VOID
}

NTSTATUS
NTAPI
LdrpInitializeExecutionOptions(PUNICODE_STRING ImagePathName, PPEB Peb, PHANDLE OptionsKey)
{
    LDR_FUNC(NTSTATUS, ImagePathName, Peb, OptionsKey)
    NTSTATUS Status;
    HANDLE KeyHandle;
    ULONG ExecuteOptions, MinimumStackCommit = 0, GlobalFlag;

    /* Return error if we were not provided a pointer where to save the options key handle */
    if (!OptionsKey) return STATUS_INVALID_HANDLE;

    /* Zero initialize the options key pointer */
    *OptionsKey = NULL;

    /* Open the options key */
    Status = LdrOpenImageFileOptionsKey(ImagePathName, 0, &KeyHandle);

    /* Save it if it was opened successfully */
    if (NT_SUCCESS(Status))
        *OptionsKey = KeyHandle;

    if (KeyHandle)
    {
        /* There are image specific options, read them starting with NXCOMPAT */
        Status = LdrQueryImageFileKeyOption(KeyHandle,
                                            L"ExecuteOptions",
                                            4,
                                            &ExecuteOptions,
                                            sizeof(ExecuteOptions),
                                            0);

        if (NT_SUCCESS(Status))
        {
            /* TODO: Set execution options for the process */
            /*
            if (ExecuteOptions == 0)
                ExecuteOptions = 1;
            else
                ExecuteOptions = 2;
            ZwSetInformationProcess(NtCurrentProcess(),
                                    ProcessExecuteFlags,
                                    &ExecuteOptions,
                                    sizeof(ULONG));*/

        }

        /* Check if this image uses large pages */
        if (Peb->ImageUsesLargePages)
        {
            /* TODO: If it does, open large page key */
            UNIMPLEMENTED;
        }

        /* Get various option values */
        LdrQueryImageFileKeyOption(KeyHandle,
                                   L"DisableHeapLookaside",
                                   REG_DWORD,
                                   &RtlpDisableHeapLookaside,
                                   sizeof(RtlpDisableHeapLookaside),
                                   NULL);

        LdrQueryImageFileKeyOption(KeyHandle,
                                   L"ShutdownFlags",
                                   REG_DWORD,
                                   &RtlpShutdownProcessFlags,
                                   sizeof(RtlpShutdownProcessFlags),
                                   NULL);

        LdrQueryImageFileKeyOption(KeyHandle,
                                   L"MinimumStackCommitInBytes",
                                   REG_DWORD,
                                   &MinimumStackCommit,
                                   sizeof(MinimumStackCommit),
                                   NULL);

        /* Update PEB's minimum stack commit if it's lower */
        if (Peb->MinimumStackCommit < MinimumStackCommit)
            Peb->MinimumStackCommit = MinimumStackCommit;

        /* Set the global flag */
        Status = LdrQueryImageFileKeyOption(KeyHandle,
                                            L"GlobalFlag",
                                            REG_DWORD,
                                            &GlobalFlag,
                                            sizeof(GlobalFlag),
                                            NULL);

        if (NT_SUCCESS(Status))
            Peb->NtGlobalFlag = GlobalFlag;
        else
            GlobalFlag = 0;

        /* Call AVRF if necessary */
        if (Peb->NtGlobalFlag & (FLG_APPLICATION_VERIFIER | FLG_HEAP_PAGE_ALLOCS))
        {
            Status = LdrpInitializeApplicationVerifierPackage(KeyHandle, Peb, TRUE, FALSE);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("AVRF: LdrpInitializeApplicationVerifierPackage failed with %08X\n", Status);
            }
        }
    }
    else
    {
        /* There are no image-specific options, so perform global initialization */
        if (Peb->NtGlobalFlag & (FLG_APPLICATION_VERIFIER | FLG_HEAP_PAGE_ALLOCS))
        {
            /* Initialize app verifier package */
            Status = LdrpInitializeApplicationVerifierPackage(KeyHandle, Peb, TRUE, FALSE);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("AVRF: LdrpInitializeApplicationVerifierPackage failed with %08X\n", Status);
            }
        }
    }

    return STATUS_SUCCESS;
    LDR_FUNC_END_RETURN
}

NTSTATUS
NTAPI
LdrpValidateImageForMp(IN PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    LDR_FUNC(NTSTATUS, LdrEntry)
    LDRP_ASSERT_MODULE_ENTRY(LdrEntry);

    NTSTATUS Status = STATUS_SUCCESS;

    if (NtCurrentPeb()->NumberOfProcessors <= 1)
        return Status;

    /*
     * Image can be converted to UP-only by replacing all LOCK x86 instruction prefixes with NOPs.
     * We can check for that by looking at the LockPrefixTable in the LoadConfig image directory.
     */

    const PIMAGE_LOAD_CONFIG_DIRECTORY Config = LdrImageDirectoryEntryToLoadConfig(LdrEntry->DllBase);

    if (!Config)
        return Status;

    if (Config->Size < RTL_SIZEOF_THROUGH_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, LockPrefixTable))
        return Status;

    BYTE** Table = (BYTE**)Config->LockPrefixTable;
    if (!Table)
        return Status;

    /*
     * Table is more like a list, containing pointers to x86 instructions, more specifically, to LOCK prefixes.
     */
    for (BYTE** Item = Table; *Item; ++Item)
    {
        // Compare instruction with NOP (0x90)
        if (**Item != 0x90u)
            continue;

        DPRINT1("LDR: \"%wZ\" has been converted to be a UP-only image.\n", &LdrEntry->BaseDllName);

        Status = STATUS_IMAGE_MP_UP_MISMATCH;
    }

    return Status;
    LDR_FUNC_END_RETURN
}

BOOLEAN
NTAPI
LdrpDisableProcessCompatGuidDetection()
{
    LDR_FUNC_MANUAL(BOOLEAN)
    UNICODE_STRING PolicyKey = RTL_CONSTANT_STRING(L"\\Registry\\MACHINE\\Software\\Policies\\Microsoft\\Windows\\AppCompat");
    UNICODE_STRING DisableDetection = RTL_CONSTANT_STRING(L"DisableCompatGuidDetection");
    OBJECT_ATTRIBUTES PolicyKeyAttributes = RTL_CONSTANT_OBJECT_ATTRIBUTES(&PolicyKey, OBJ_CASE_INSENSITIVE);
    KEY_VALUE_PARTIAL_INFORMATION KeyInfo;
    ULONG ResultLength;
    NTSTATUS Status;
    HANDLE KeyHandle;

    Status = NtOpenKey(&KeyHandle, KEY_QUERY_VALUE, &PolicyKeyAttributes);
    if (NT_SUCCESS(Status))
    {
        Status = NtQueryValueKey(KeyHandle,
                                 &DisableDetection,
                                 KeyValuePartialInformation,
                                 &KeyInfo,
                                 sizeof(KeyInfo),
                                 &ResultLength);
        NtClose(KeyHandle);
        if ((NT_SUCCESS(Status)) &&
            (KeyInfo.Type == REG_DWORD) &&
            (KeyInfo.DataLength == sizeof(ULONG)) &&
            (KeyInfo.Data[0] == TRUE))
        {
            return TRUE;
        }
    }
    return FALSE;
    LDR_FUNC_END_IMPL(return FALSE)
}


void
NTAPI
LdrpInitializeProcessCompat(PVOID pProcessActctx, PVOID* pOldShimData)
{
    LDR_FUNC_VOID(pProcessActctx, pOldShimData)
    static const struct
    {
        const GUID* Guid;
        const DWORD Version;
    } KnownCompatGuids[] = {
        { &COMPAT_GUID_WIN10, _WIN32_WINNT_WIN10 },
        { &COMPAT_GUID_WIN81, _WIN32_WINNT_WINBLUE },
        { &COMPAT_GUID_WIN8, _WIN32_WINNT_WIN8 },
        { &COMPAT_GUID_WIN7, _WIN32_WINNT_WIN7 },
        { &COMPAT_GUID_VISTA, _WIN32_WINNT_VISTA },
    };

    ULONG Buffer[(sizeof(COMPATIBILITY_CONTEXT_ELEMENT) * 10 + sizeof(ACTIVATION_CONTEXT_COMPATIBILITY_INFORMATION)) / sizeof(ULONG)];
    ACTIVATION_CONTEXT_COMPATIBILITY_INFORMATION* ContextCompatInfo;
    SIZE_T SizeRequired;
    NTSTATUS Status;
    DWORD n, cur;
    auto* pShimData = static_cast<ReactOS_ShimData*>(*pOldShimData);

    if (pShimData)
    {
        if (pShimData->dwMagic != REACTOS_SHIMDATA_MAGIC ||
            pShimData->dwSize != sizeof(ReactOS_ShimData))
        {
            DPRINT1("LdrpInitializeProcessCompat: Corrupt pShimData (0x%x, %u)\n", pShimData->dwMagic, pShimData->dwSize);
            return;
        }
        if (pShimData->dwRosProcessCompatVersion)
        {
            if (pShimData->dwRosProcessCompatVersion == REACTOS_COMPATVERSION_IGNOREMANIFEST)
            {
                DPRINT1("LdrpInitializeProcessCompat: ProcessCompatVersion set to ignore manifest\n");
            }
            else
            {
                DPRINT1("LdrpInitializeProcessCompat: ProcessCompatVersion already set to 0x%x\n", pShimData->dwRosProcessCompatVersion);
            }
            return;
        }
    }

    SizeRequired = sizeof(Buffer);
    Status = RtlQueryInformationActivationContext(RTL_QUERY_ACTIVATION_CONTEXT_FLAG_NO_ADDREF,
                                                  pProcessActctx,
                                                  NULL,
                                                  CompatibilityInformationInActivationContext,
                                                  Buffer,
                                                  sizeof(Buffer),
                                                  &SizeRequired);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("LdrpInitializeProcessCompat: Unable to query process actctx with status 0x%08lX\n", Status);
        return;
    }

    ContextCompatInfo = (ACTIVATION_CONTEXT_COMPATIBILITY_INFORMATION*)Buffer;
    /* No Compatibility elements present, bail out */
    if (ContextCompatInfo->ElementCount == 0)
        return;

    /* Search for known GUIDs, starting from oldest to newest.
       Note that on Windows it is somewhat reversed, starting from the latest known
       version, going down. But we are not Windows, trying to allow a lower version,
       we are ReactOS trying to fake a higher version. So we interpret what Windows
       does as "try the closest version to the actual version", so we start with the
       lowest version, which is closest to Windows 2003, which we mostly are. */
    for (cur = RTL_NUMBER_OF(KnownCompatGuids) - 1; cur != -1; --cur)
    {
        for (n = 0; n < ContextCompatInfo->ElementCount; ++n)
        {
            if (ContextCompatInfo->Elements[n].Type == ACTCTX_COMPATIBILITY_ELEMENT_TYPE_OS &&
                RtlCompareMemory(&ContextCompatInfo->Elements[n].Id, KnownCompatGuids[cur].Guid, sizeof(GUID)) == sizeof(GUID))
            {
                if (LdrpDisableProcessCompatGuidDetection())
                {
                    DPRINT1("LdrpInitializeProcessCompat: Not applying automatic fix for winver 0x%x due to policy\n", KnownCompatGuids[cur].Version);
                    return;
                }

                /* If this process did not need shim data before, allocate and store it */
                if (pShimData == NULL)
                {
                    PPEB Peb = NtCurrentPeb();

                    ASSERT(Peb->pShimData == NULL);
                    pShimData = new ReactOS_ShimData;

                    if (!pShimData)
                    {
                        DPRINT1("LdrpInitializeProcessCompat: Unable to allocated %u bytes\n", sizeof(*pShimData));
                        return;
                    }

                    pShimData->dwSize = sizeof(*pShimData);
                    pShimData->dwMagic = REACTOS_SHIMDATA_MAGIC;

                    Peb->pShimData = pShimData;
                    *pOldShimData = pShimData;
                }

                /* Store the lowest found version, and bail out. */
                pShimData->dwRosProcessCompatVersion = KnownCompatGuids[cur].Version;
                DPRINT1("LdrpInitializeProcessCompat: Found guid for winver 0x%x in manifest from %wZ\n",
                        KnownCompatGuids[cur].Version,
                        &(NtCurrentPeb()->ProcessParameters->ImagePathName));
                return;
            }
        }
    }
    LDR_FUNC_END_VOID
}

void
NTAPI
LdrpInitializeDotLocalSupport(PRTL_USER_PROCESS_PARAMETERS ProcessParameters)
{
    LDR_FUNC_VOID(ProcessParameters)
    UNICODE_STRING ImagePathName = ProcessParameters->ImagePathName;
    WCHAR LocalBuffer[MAX_PATH];
    UNICODE_STRING DotLocal;
    NTSTATUS Status;
    ULONG RequiredSize;

    RequiredSize = ImagePathName.Length + LdrpDotLocal.Length + sizeof(UNICODE_NULL);
    if (RequiredSize <= sizeof(LocalBuffer))
    {
        RtlInitEmptyUnicodeString(&DotLocal, LocalBuffer, sizeof(LocalBuffer));
    }
    else if (RequiredSize <= UNICODE_STRING_MAX_BYTES)
    {
        DotLocal.Buffer = (PWSTR)RtlAllocateHeap(RtlGetProcessHeap(), 0, RequiredSize);
        DotLocal.Length = 0;
        DotLocal.MaximumLength = RequiredSize;
        if (!DotLocal.Buffer)
            DPRINT1("LDR: Failed to allocate memory for .local check\n");
    }
    else
    {
        DotLocal.Buffer = NULL;
        DotLocal.Length = 0;
        DotLocal.MaximumLength = 0;
        DPRINT1("LDR: String too big for .local check\n");
    }

    if (DotLocal.Buffer)
    {
        Status = RtlAppendUnicodeStringToString(&DotLocal, &ImagePathName);
        ASSERT(NT_SUCCESS(Status));
        if (NT_SUCCESS(Status))
        {
            Status = RtlAppendUnicodeStringToString(&DotLocal, &LdrpDotLocal);
            ASSERT(NT_SUCCESS(Status));
        }

        if (NT_SUCCESS(Status))
        {
            if (RtlDoesFileExists_UstrEx(&DotLocal, TRUE))
            {
                ProcessParameters->Flags |= RTL_USER_PROCESS_PARAMETERS_PRIVATE_DLL_PATH;
            }
        }
        else
        {
            DPRINT1("LDR: Failed to append: 0x%lx\n", Status);
        }

        if (DotLocal.Buffer != LocalBuffer)
        {
            RtlFreeHeap(RtlGetProcessHeap(), 0, DotLocal.Buffer);
        }
    }
    LDR_FUNC_END_VOID
}

void
FASTCALL
LdrpInitializeProcessCSD()
{
    LDR_FUNC_VOID_MANUAL()
    PPEB Peb = NtCurrentPeb();

    // FIXME: Should be done by Application Compatibility features,
    // by reading the registry, etc...
    // For now, this is the old code from ntdll!RtlGetVersion().
    RtlInitEmptyUnicodeString(&Peb->CSDVersion, NULL, 0);
    if (((Peb->OSCSDVersion >> 8) & 0xFF) != 0)
    {
        WCHAR szCSDVersion[128];
        LONG i;
        USHORT Length = (USHORT)ARRAYSIZE(szCSDVersion) - 1;
        i = _snwprintf(szCSDVersion, Length,
                       L"Service Pack %d",
                       (Peb->OSCSDVersion >> 8) & 0xFF);
        if (i < 0)
        {
            /* Null-terminate if it was overflowed */
            szCSDVersion[Length] = UNICODE_NULL;
        }

        Length *= sizeof(WCHAR);
        Peb->CSDVersion.Buffer = (PWSTR)RtlAllocateHeap(RtlGetProcessHeap(),
                                                 0,
                                                 Length + sizeof(UNICODE_NULL));
        if (Peb->CSDVersion.Buffer)
        {
            Peb->CSDVersion.Length = Length;
            Peb->CSDVersion.MaximumLength = Length + sizeof(UNICODE_NULL);

            RtlCopyMemory(Peb->CSDVersion.Buffer,
                          szCSDVersion,
                          Peb->CSDVersion.MaximumLength);
            Peb->CSDVersion.Buffer[Peb->CSDVersion.Length / sizeof(WCHAR)] = UNICODE_NULL;
        }
    }
    LDR_FUNC_END_VOID
}

void
LdrpSetProcessExecuteOptions(IN KEXECUTE_OPTIONS ExecuteOptions)
{
    LDR_FUNC_VOID(ExecuteOptions)
    ULONG ProcessInformation = ExecuteOptions.ExecuteOptionsNV;

    ASSERT(ProcessInformation);

    /* Let Mm know */
    NtSetInformationProcess(NtCurrentProcess(),
                            ProcessExecuteFlags,
                            &ProcessInformation,
                            sizeof(ULONG));
    LDR_FUNC_END_VOID
}

void
LdrpApplyProcessPermanentNoExecuteMitigationPolicy()
{
    LDR_FUNC_VOID_MANUAL()
    const UCHAR Policy = SharedUserData->NXSupportPolicy;
    KEXECUTE_OPTIONS ExecuteOptions = {
        .ExecuteOptionsNV = 0
    };

    switch (Policy)
    {
        case NX_SUPPORT_POLICY_ALWAYSON:
        case NX_SUPPORT_POLICY_OPTIN:
        case NX_SUPPORT_POLICY_OPTOUT:
        case NX_SUPPORT_POLICY_ALWAYSOFF:
            break;

        DEFAULT_UNREACHABLE;
    }

    if (Policy == NX_SUPPORT_POLICY_ALWAYSON || LdrpNXProcessPermanent)
    {
        ExecuteOptions.ExecuteDisable = TRUE;
        ExecuteOptions.DisableThunkEmulation = TRUE;
    }
    else if (Policy == NX_SUPPORT_POLICY_ALWAYSOFF)
    {
        ExecuteOptions.ExecuteEnable = TRUE;
    }

    if (ExecuteOptions.ExecuteOptionsNV)
    {
        LdrpNXProcessPermanent = TRUE;
        ExecuteOptions.Permanent = TRUE;

        LdrpSetProcessExecuteOptions(ExecuteOptions);
    }
    LDR_FUNC_END_VOID
}

BOOLEAN
LdrpCheckForSecuROMImage(PIMAGE_NT_HEADERS NtHeader)
{
    LDR_FUNC(BOOLEAN, NtHeader)
    /* todo */
    return FALSE;
    LDR_FUNC_END_IMPL(return FALSE);
}

BOOLEAN
LdrpEntrySectionValid(PIMAGE_NT_HEADERS NtHeader)
{
    LDR_FUNC(BOOLEAN, NtHeader)
    /* todo */
    return TRUE;
    LDR_FUNC_END_IMPL(return FALSE);
}

BOOLEAN
LdrpCheckForSafeDiscImage(PIMAGE_NT_HEADERS NtHeader)
{
    LDR_FUNC(BOOLEAN, NtHeader)
    /* todo */
    return FALSE;
    LDR_FUNC_END_IMPL(return FALSE);
}

void
LdrpApplyProcessNoExecuteMitigationPolicy()
{
    LDR_FUNC_VOID_MANUAL()
    const UCHAR Policy = SharedUserData->NXSupportPolicy;
    const PIMAGE_NT_HEADERS NtHeader = LdrpAppHeaders;
    KEXECUTE_OPTIONS ExecuteOptions = {
        .ExecuteOptionsNV = 0
    };

    if (LdrpNXProcessPermanent)
        return;

    switch (Policy)
    {
        case NX_SUPPORT_POLICY_ALWAYSON:
        case NX_SUPPORT_POLICY_OPTIN:
        case NX_SUPPORT_POLICY_OPTOUT:
        case NX_SUPPORT_POLICY_ALWAYSOFF:
            break;

        DEFAULT_UNREACHABLE;
    }

    if (Policy == NX_SUPPORT_POLICY_OPTIN &&
        !(NtCurrentPeb()->ProcessParameters->Flags & RTL_USER_PROCESS_PARAMETERS_NX) &&
        NtHeader->OptionalHeader.Subsystem != IMAGE_SUBSYSTEM_NATIVE)
    {
        ExecuteOptions.ExecuteEnable = TRUE;
    }
    else if (LdrpCheckForSecuROMImage(NtHeader) || !LdrpEntrySectionValid(NtHeader) ||
        LdrpCheckForSafeDiscImage(NtHeader))
    {
        ExecuteOptions.ExecuteEnable = TRUE;
    }

    if (ExecuteOptions.ExecuteOptionsNV)
    {
        LdrpSetProcessExecuteOptions(ExecuteOptions);
    }
    LDR_FUNC_END_VOID
}

NTSTATUS
NTAPI
LdrpInitializeProcess(IN PCONTEXT Context,
                      IN PVOID SystemDllBaseAddress)
{
    LDR_FUNC(NTSTATUS, Context, SystemDllBaseAddress)
    RTL_HEAP_PARAMETERS HeapParameters;
    ULONG ComSectionSize;
    ANSI_STRING BaseProcessInitPostImportName = RTL_CONSTANT_STRING("BaseProcessInitPostImport");
    ANSI_STRING BaseQueryModuleDataName = RTL_CONSTANT_STRING("BaseQueryModuleData");
    PVOID OldShimData = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    //UNICODE_STRING LocalFileName, FullImageName;
    HANDLE SymLinkHandle;
    //ULONG DebugHeapOnly;
    UNICODE_STRING CommandLine, NtSystemRoot, ImagePathName, FullPath, ImageFileName, KnownDllString;
    PPEB Peb = NtCurrentPeb();
    BOOLEAN FreeCurDir = FALSE;
    //HANDLE CompatKey;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    //LPWSTR ImagePathBuffer;
    UNICODE_STRING CurrentDirectory;
    HANDLE OptionsKey;
    ULONG HeapFlags;
    LPWSTR NtDllName = NULL;
    NTSTATUS Status;
    NLSTABLEINFO NlsTable;
    PIMAGE_LOAD_CONFIG_DIRECTORY LoadConfig = NULL;
    PTEB Teb = NtCurrentTeb();
    PLIST_ENTRY ListHead;
    PLIST_ENTRY NextEntry;
    ULONG i;
    PWSTR ImagePath;
    ULONG DebugProcessHeapOnly = 0;
    PWCHAR Current;
    LDRP_LOAD_CONTEXT_FLAGS EmptyLoadContextFlags = {0};

    /* Set a NULL SEH Filter */
    RtlSetUnhandledExceptionFilter(NULL);

    /* Prepare spinning on SRW locks on MP systems */
    SRWLockSpinCycleCount = Peb->NumberOfProcessors > 1 ? 10240 : 0;

    /* Get the image path */
    ImagePath = Peb->ProcessParameters->ImagePathName.Buffer;

    /* Check if it's not normalized */
    if (!(Peb->ProcessParameters->Flags & RTL_USER_PROCESS_PARAMETERS_NORMALIZED))
    {
        /* Normalize it*/
        ImagePath = (PWSTR)((ULONG_PTR)ImagePath + (ULONG_PTR)Peb->ProcessParameters);
    }

    /* Create a unicode string for the Image Path */
    ImagePathName.Length = Peb->ProcessParameters->ImagePathName.Length;
    ImagePathName.MaximumLength = ImagePathName.Length + sizeof(WCHAR);
    ImagePathName.Buffer = ImagePath;

    /* Get the NT Headers */
    LdrpAppHeaders = RtlImageNtHeader(Peb->ImageBaseAddress);

    if (SharedUserData->NXSupportPolicy != NX_SUPPORT_POLICY_ALWAYSOFF && LdrpAppHeaders->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_NX_COMPAT)
    {
        LdrpNXProcessPermanent = TRUE;
    }

    /* Get the execution options */
    Status = LdrpInitializeExecutionOptions(&ImagePathName, Peb, &OptionsKey);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("LDR: Initializing the execution options for the process 0x%lX failed with status 0x%08lX\n",
                Teb->ClientId.UniqueProcess, Status);
        return Status;
    }

#if defined(_M_AMD64) || defined(_WIN64)
    {
        ULONG_PTR Wow64Information = 0;
        Status = NtQueryInformationProcess(NtCurrentProcess(), ProcessWow64Information,
                                           &Wow64Information, sizeof(ULONG_PTR), NULL);

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("LDR: Querying WoW64 information for the process 0x%lX failed with status 0x%08lX\n",
                    Teb->ClientId.UniqueProcess, Status);
            return Status;
        }

        UseWOW64 = Wow64Information != 0;
    }
#endif

    BOOL ManagedImageArchConverted = FALSE;
    BOOL ImageHasNativeCode = TRUE;

#if defined(_M_AMD64) || defined(_WIN64)
    if (!UseWOW64 && LdrpAppHeaders && LdrpAppHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        UseCOR = TRUE;

        ManagedImageArchConverted = TRUE;

        Status = LdrpCorFixupImage(Peb->ImageBaseAddress);

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("LDR: Managed x86->x86_64 image fixup for the process 0x%lX failed with status 0x%08lX\n",
                    Teb->ClientId.UniqueProcess, Status);
            return Status;
        }
    }
#endif

    if (!UseWOW64 && !UseCOR)
    {
        PIMAGE_COR20_HEADER Cor20Header = (PIMAGE_COR20_HEADER)RtlImageDirectoryEntryToData(Peb->ImageBaseAddress,
            TRUE,
            IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR,
            &ComSectionSize);

        /* Check if this is a .NET executable */
        if (Cor20Header)
        {
            UseCOR = TRUE;
            
            ImageHasNativeCode = !(Cor20Header->Flags & COMIMAGE_FLAGS_ILONLY);
        }
    }

    /* Save the NTDLL Base address */
    LdrpSystemDllBase = SystemDllBaseAddress;

    /* If this is a Native Image */
    if (LdrpAppHeaders->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_NATIVE)
    {
        /* Then do DLL Validation */
        LdrpDllValidation = TRUE;
    }

    /* Save the old Shim Data */
    if (!UseWOW64)
        OldShimData = Peb->pShimData;

    /* ReactOS specific: do not clear it. (Windows starts doing the same in later versions) */
    //Peb->pShimData = NULL;

    /* Save the number of processors and CS Timeout */
    RtlpTimeout = Peb->CriticalSectionTimeout;

    /* Normalize the parameters */
    ProcessParameters = RtlNormalizeProcessParams(Peb->ProcessParameters);
    if (ProcessParameters)
    {
        /* Save the Image and Command Line Names */
        ImageFileName = ProcessParameters->ImagePathName;
        CommandLine = ProcessParameters->CommandLine;
    }
    else
    {
        /* It failed, initialize empty strings */
        RtlInitUnicodeString(&ImageFileName, NULL);
        RtlInitUnicodeString(&CommandLine, NULL);
    }

    /* Initialize NLS data */
    RtlInitNlsTables((PUSHORT)Peb->AnsiCodePageData,
                     (PUSHORT)Peb->OemCodePageData,
                     (PUSHORT)Peb->UnicodeCaseTableData,
                     &NlsTable);

    /* Reset NLS Translations */
    RtlResetRtlTranslations(&NlsTable);

    /* Get the Image Config Directory */
    if (!UseWOW64 && !(UseCOR && ManagedImageArchConverted))
        LoadConfig = LdrImageDirectoryEntryToLoadConfig(Peb->ImageBaseAddress);

    /* Setup the Heap Parameters */
    RtlZeroMemory(&HeapParameters, sizeof(HeapParameters));
    HeapFlags = HEAP_GROWABLE;
    HeapParameters.Length = sizeof(HeapParameters);

    /* Check if we have Configuration Data */
#define VALID_CONFIG_FIELD(Name) (LoadConfig->Size >= RTL_SIZEOF_THROUGH_FIELD(IMAGE_LOAD_CONFIG_DIRECTORY, Name))
    /* The 'original' load config ends after SecurityCookie */
    if (LoadConfig)
    {
        DPRINT1("Applying LOAD_CONFIG\n");

        if (VALID_CONFIG_FIELD(GlobalFlagsClear) && LoadConfig->GlobalFlagsClear)
            Peb->NtGlobalFlag &= ~LoadConfig->GlobalFlagsClear;

        if (VALID_CONFIG_FIELD(GlobalFlagsSet) && LoadConfig->GlobalFlagsSet)
            Peb->NtGlobalFlag |= LoadConfig->GlobalFlagsSet;

        if (VALID_CONFIG_FIELD(CriticalSectionDefaultTimeout) && LoadConfig->CriticalSectionDefaultTimeout)
            RtlpTimeout.QuadPart = Int32x32To64(LoadConfig->CriticalSectionDefaultTimeout, -10000);

        if (VALID_CONFIG_FIELD(DeCommitFreeBlockThreshold) && LoadConfig->DeCommitFreeBlockThreshold)
            HeapParameters.DeCommitFreeBlockThreshold = LoadConfig->DeCommitFreeBlockThreshold;

        if (VALID_CONFIG_FIELD(DeCommitTotalFreeThreshold) && LoadConfig->DeCommitTotalFreeThreshold)
            HeapParameters.DeCommitTotalFreeThreshold = LoadConfig->DeCommitTotalFreeThreshold;

        if (VALID_CONFIG_FIELD(MaximumAllocationSize) && LoadConfig->MaximumAllocationSize)
            HeapParameters.MaximumAllocationSize = LoadConfig->MaximumAllocationSize;

        if (VALID_CONFIG_FIELD(VirtualMemoryThreshold) && LoadConfig->VirtualMemoryThreshold)
            HeapParameters.VirtualMemoryThreshold = LoadConfig->VirtualMemoryThreshold;

        if (VALID_CONFIG_FIELD(ProcessHeapFlags) && LoadConfig->ProcessHeapFlags)
            HeapFlags = LoadConfig->ProcessHeapFlags & ~0xF000;
    }
#undef VALID_CONFIG_FIELD

    /* Check for custom affinity mask */
    if (Peb->ImageProcessAffinityMask)
    {
        /* Set it */
        Status = NtSetInformationProcess(NtCurrentProcess(),
                                         ProcessAffinityMask,
                                         &Peb->ImageProcessAffinityMask,
                                         sizeof(Peb->ImageProcessAffinityMask));
    }

    /* Check if verbose debugging (ShowSnaps) was requested */
    if (Peb->NtGlobalFlag & FLG_SHOW_LDR_SNAPS)
    {
        DPRINT1("TODO (andrew.boyarshin): Fix logging levels.\n");
    }

#if 1
    LdrpDebugFlags.LogWarning = LdrpDebugFlags.LogInformation = TRUE;
    LdrpDebugFlags.LogDebug = LdrpDebugFlags.LogTrace = TRUE;
#endif
    LdrpDebugFlags.BreakInDebugger = TRUE;

    /* Start verbose debugging messages right now if they were requested */
    if (LdrpDebugFlags.LogInformation)
    {
        DPRINT1("LDR: PID: " LDR_HEXPTR_FMT " started - '%wZ'\n",
                Teb->ClientId.UniqueProcess,
                &CommandLine);
    }

    /* If the timeout is too long */
    if (RtlpTimeout.QuadPart < Int32x32To64(3600, -10000000))
    {
        /* Then disable CS Timeout */
        RtlpTimeoutDisable = TRUE;
    }

    /* Initialize Critical Section Data */
    RtlpInitDeferredCriticalSection();

    /* Initialize the Loader Lock */
    ASSERT(RtlpCritSectInitialized);

    InsertTailList(&RtlCriticalSectionList, &LdrpLoaderLock.DebugInfo->ProcessLocksList);
    LdrpLoaderLock.DebugInfo->CriticalSection = &LdrpLoaderLock;

    /* Initialize VEH Call lists */
    RtlpInitializeVectoredExceptionHandling();

    /* Set TLS/FLS Bitmap data */
    Peb->TlsBitmap = &TlsBitMap;
    Peb->TlsExpansionBitmap = &TlsExpansionBitMap;

    /* Initialize FLS */
    RtlpFlsInitialize();

    /* Initialize TLS Bitmap */
    RtlInitializeBitMap(&TlsBitMap,
                        Peb->TlsBitmapBits,
                        TLS_MINIMUM_AVAILABLE);
    RtlSetBit(&TlsBitMap, 0);
    RtlInitializeBitMap(&TlsExpansionBitMap,
                        Peb->TlsExpansionBitmapBits,
                        TLS_EXPANSION_SLOTS);
    RtlSetBit(&TlsExpansionBitMap, 0);

    /* Initialize the Hash Table */
    for (i = 0; i < LDR_HASH_TABLE_ENTRIES; i++)
    {
        InitializeListHead(&LdrpHashTable[i]);
    }

    /* Check if User Stack Trace Database support was requested */
    if (Peb->NtGlobalFlag & FLG_USER_STACK_TRACE_DB)
    {
        DPRINT1("We don't support user stack trace databases yet\n");
    }

    /* Setup Fast PEB Lock */
    RtlInitializeCriticalSection(&FastPebLock);
    Peb->FastPebLock = &FastPebLock;
    //Peb->FastPebLockRoutine = (PPEBLOCKROUTINE)RtlEnterCriticalSection;
    //Peb->FastPebUnlockRoutine = (PPEBLOCKROUTINE)RtlLeaveCriticalSection;

    /* Setup Callout Lock and Notification list */
    //RtlInitializeCriticalSection(&RtlpCalloutEntryLock);
    InitializeListHead(&LdrpDllNotificationList);

    /* For old executables, use 16-byte aligned heap */
    if ((LdrpAppHeaders->OptionalHeader.MajorSubsystemVersion <= 3) &&
        (LdrpAppHeaders->OptionalHeader.MinorSubsystemVersion < 51))
    {
        HeapFlags |= HEAP_CREATE_ALIGN_16;
    }

    DPRINT1("ALARM: REMOVE HEAP DEBUGGING\n");
    HeapFlags |= HEAP_VALIDATE_ALL_ENABLED;
    HeapFlags |= HEAP_VALIDATE_PARAMETERS_ENABLED;
    HeapFlags |= HEAP_FREE_CHECKING_ENABLED;
    HeapFlags |= HEAP_TAIL_CHECKING_ENABLED;

    /* Setup the Heap */
    RtlInitializeHeapManager();
    Peb->ProcessHeap = RtlCreateHeap(HeapFlags,
                                     NULL,
                                     LdrpAppHeaders->OptionalHeader.SizeOfHeapReserve,
                                     LdrpAppHeaders->OptionalHeader.SizeOfHeapCommit,
                                     NULL,
                                     &HeapParameters);

    if (!Peb->ProcessHeap)
    {
        DPRINT1("Failed to create process heap\n");
        return STATUS_NO_MEMORY;
    }

    LdrpHeap = Peb->ProcessHeap;

    LdrpHeapAllocProc = &LdrpHeapAlloc;
    LdrpHeapFreeProc = &LdrpHeapFree;
    LdrpHeapReAllocProc = &LdrpHeapReAlloc;

    /* Allocate an Activation Context Stack */
    Status = RtlAllocateActivationContextStack(&Teb->ActivationContextStackPointer);
    if (!NT_SUCCESS(Status)) return Status;

    /* Check for Debug Heap */
    if (OptionsKey)
    {
        /* Query the setting */
        Status = LdrQueryImageFileKeyOption(OptionsKey,
                                            L"DebugProcessHeapOnly",
                                            REG_DWORD,
                                            &DebugProcessHeapOnly,
                                            sizeof(ULONG),
                                            NULL);

        if (NT_SUCCESS(Status))
        {
            /* Reset DPH if requested */
            if (RtlpPageHeapEnabled && DebugProcessHeapOnly)
            {
                RtlpDphGlobalFlags &= ~DPH_FLAG_DLL_NOTIFY;
                RtlpPageHeapEnabled = FALSE;
            }
        }
    }

    /* Build the NTDLL Path */
    FullPath.Buffer = StringBuffer;
    FullPath.Length = 0;
    FullPath.MaximumLength = sizeof(StringBuffer);
    RtlInitUnicodeString(&NtSystemRoot, SharedUserData->NtSystemRoot);

    ASSERT(NT_SUCCESS(RtlAppendUnicodeStringToString(&FullPath, &NtSystemRoot)));
    ASSERT(NT_SUCCESS(RtlAppendUnicodeStringToString(&FullPath, &SlashSystem32SlashString)));

    /* Open the Known DLLs directory */
    RtlInitUnicodeString(&KnownDllString, L"\\KnownDlls");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KnownDllString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = ZwOpenDirectoryObject(&LdrpKnownDllDirectoryHandle,
                                   DIRECTORY_QUERY | DIRECTORY_TRAVERSE,
                                   &ObjectAttributes);

    /* Check if it exists */
    if (NT_SUCCESS(Status))
    {
        /* Open the Known DLLs Path */
        RtlInitUnicodeString(&KnownDllString, L"KnownDllPath");
        InitializeObjectAttributes(&ObjectAttributes,
                                   &KnownDllString,
                                   OBJ_CASE_INSENSITIVE,
                                   LdrpKnownDllDirectoryHandle,
                                   NULL);
        Status = NtOpenSymbolicLinkObject(&SymLinkHandle,
                                          SYMBOLIC_LINK_QUERY,
                                          &ObjectAttributes);
        if (NT_SUCCESS(Status))
        {
            /* Query the path */
            LdrpKnownDllPath.Length = 0;
            LdrpKnownDllPath.MaximumLength = sizeof(LdrpKnownDllPathBuffer);
            LdrpKnownDllPath.Buffer = LdrpKnownDllPathBuffer;
            Status = ZwQuerySymbolicLinkObject(SymLinkHandle, &LdrpKnownDllPath, NULL);
            NtClose(SymLinkHandle);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("failed call to ZwQuerySymbolicLinkObject with status 0x%08lX\n", Status);
                return Status;
            }
        }
    }

    /* Check if we failed */
    if (!NT_SUCCESS(Status))
    {
        /* Assume System32 */
        LdrpKnownDllDirectoryHandle = NULL;
        RtlInitUnicodeString(&LdrpKnownDllPath, StringBuffer);
        LdrpKnownDllPath.Length -= sizeof(WCHAR);
    }

    /* If we have process parameters, get the default path and current path */
    if (ProcessParameters)
    {
        /* Check if we have a Dll Path */
        if (ProcessParameters->DllPath.Length)
        {
            /* Get the path */
            LdrpDefaultPath = *(PUNICODE_STRING)&ProcessParameters->DllPath;
        }
        else
        {
            /* We need a valid path */
            DPRINT1("No valid DllPath was given!\n");
            return STATUS_INVALID_PARAMETER;
        }

        /* Set the current directory */
        CurrentDirectory = ProcessParameters->CurrentDirectory.DosPath;

        /* Check if it's empty or invalid */
        if ((!CurrentDirectory.Buffer) ||
            (CurrentDirectory.Buffer[0] == UNICODE_NULL) ||
            (!CurrentDirectory.Length))
        {
            /* Allocate space for the buffer */
            CurrentDirectory.Buffer = (PWSTR)RtlAllocateHeap(Peb->ProcessHeap,
                                                             0,
                                                             3 * sizeof(WCHAR) +
                                                             sizeof(UNICODE_NULL));
            if (!CurrentDirectory.Buffer)
            {
                DPRINT1("LDR: LdrpInitializeProcess - unable to allocate current working directory buffer\n");
                // FIXME: And what?
            }

            /* Copy the drive of the system root */
            RtlMoveMemory(CurrentDirectory.Buffer,
                          SharedUserData->NtSystemRoot,
                          3 * sizeof(WCHAR));
            CurrentDirectory.Buffer[3] = UNICODE_NULL;
            CurrentDirectory.Length = 3 * sizeof(WCHAR);
            CurrentDirectory.MaximumLength = CurrentDirectory.Length + sizeof(WCHAR);

            FreeCurDir = TRUE;
            DPRINT("Using dynamically allocd curdir\n");
        }
        else
        {
            /* Use the local buffer */
            DPRINT("Using local system root\n");
        }
    }

    /* Setup Loader Data */
    Peb->Ldr = &PebLdr;
    InitializeListHead(&PebLdr.InLoadOrderModuleList);
    InitializeListHead(&PebLdr.InMemoryOrderModuleList);
    InitializeListHead(&PebLdr.InInitializationOrderModuleList);
    PebLdr.Length = sizeof(PEB_LDR_DATA);
    PebLdr.Initialized = TRUE;

    /* Now add an entry for NTDLL */
    LdrpNtDllDataTableEntry = LdrpAllocateModuleEntry(NULL);
    if (!LdrpNtDllDataTableEntry)
    {
        Status = STATUS_NO_MEMORY;
        DPRINT1("Failed to allocate LDR_DATA_TABLE_ENTRY for NTDLL, aborting process initialization.\n");
        return Status;
    }

    LdrpNtDllDataTableEntry->DllBase = SystemDllBaseAddress;
    LdrpNtDllDataTableEntry->ProcessStaticImport = TRUE;
    LdrpNtDllDataTableEntry->ImageDll = TRUE;
    LdrpNtDllDataTableEntry->DdagNode->LoadCount = LDR_LOADCOUNT_MAX;
    LdrpNtDllDataTableEntry->DdagNode->State = LdrModulesMapped;
    LdrpNtDllDataTableEntry->LoadReason = LoadReasonStaticDependency;
    LdrpNtDllDataTableEntry->EntryPointActivationContext = 0;

    LdrpNtDllDataTableEntry->FullDllName.Length = FullPath.Length;
    LdrpNtDllDataTableEntry->FullDllName.MaximumLength = FullPath.MaximumLength;
    LdrpNtDllDataTableEntry->FullDllName.Buffer = StringBuffer;
    RtlAppendUnicodeStringToString(&LdrpNtDllDataTableEntry->FullDllName, &NtDllString);

    LdrpNtDllDataTableEntry->BaseDllName = NtDllString;

    /* Processing done, insert it */
    LdrpInsertDataTableEntry(LdrpNtDllDataTableEntry);
    LdrpInsertModuleToIndex(LdrpNtDllDataTableEntry, RtlImageNtHeader(LdrpSystemDllBase));

    /* Link the Init Order List */
    InsertHeadList(&Peb->Ldr->InInitializationOrderModuleList,
                   &LdrpNtDllDataTableEntry->InInitializationOrderLinks);

    RtlpCheckListEntry(&PebLdr.InLoadOrderModuleList);
    RtlpCheckListEntry(&PebLdr.InMemoryOrderModuleList);
    RtlpCheckListEntry(&PebLdr.InInitializationOrderModuleList);

    /* Allocate a data entry for the Image */
    {
        auto* LoadContext = new LDRP_LOAD_CONTEXT;
        if (!LoadContext)
        {
            Status = STATUS_NO_MEMORY;
            DPRINT1("Failed to allocate LDRP_LOAD_CONTEXT for LdrpImageEntry, aborting process initialization.\n");
            return Status;
        }

        LDRP_MARK_LOAD_CONTEXT(LoadContext);

        LdrpImageEntry = LdrpAllocateModuleEntry(LoadContext);
    }

    /* Set it up */
    LdrpImageEntry->ProcessStaticImport = TRUE;
    LdrpImageEntry->DdagNode->LoadCount = LDR_LOADCOUNT_MAX;
    LdrpImageEntry->EntryPointActivationContext = 0;
    LdrpImageEntry->LoadReason = LoadReasonDynamicLoad;
    LdrpImageEntry->FullDllName = ImageFileName;

    if (UseCOR)
    {
        LdrpImageEntry->CorImage = TRUE;
        if (!ImageHasNativeCode)
            LdrpImageEntry->CorILOnly = TRUE;
    }

    /* Check if the name is empty */
    if (!ImageFileName.Buffer[0])
    {
        /* Use the same Base name */
        LdrpImageEntry->BaseDllName = LdrpImageEntry->FullDllName;
    }
    else
    {
        /* Find the last slash */
        Current = ImageFileName.Buffer;
        while (*Current)
        {
            if (*Current++ == '\\')
            {
                /* Set this path */
                NtDllName = Current;
            }
        }

        /* Did we find anything? */
        if (!NtDllName)
        {
            /* Use the same Base name */
            LdrpImageEntry->BaseDllName = LdrpImageEntry->FullDllName;
        }
        else
        {
            /* Setup the name */
            LdrpImageEntry->BaseDllName.Length = (USHORT)((ULONG_PTR)ImageFileName.Buffer + ImageFileName.Length - (ULONG_PTR)NtDllName);
            LdrpImageEntry->BaseDllName.MaximumLength = LdrpImageEntry->BaseDllName.Length + sizeof(WCHAR);

            const USHORT I = ImageFileName.Length - LdrpImageEntry->BaseDllName.Length;
            LdrpImageEntry->BaseDllName.Buffer = (PWSTR)PTR_ADD_OFFSET(ImageFileName.Buffer, I);
        }
    }

    LDRP_PATH_SEARCH_CONTEXT PathSearchContext = {0};
    PathSearchContext.DllSearchPath = LdrpDefaultPath;

    LdrpImageEntry->LoadContext->Flags.HasFullPath = TRUE;
    LdrpImageEntry->LoadContext->LoadStatus = &Status;
    LdrpImageEntry->LoadContext->DllName = LdrpImageEntry->FullDllName;
    LdrpImageEntry->LoadContext->PathSearchContext = &PathSearchContext;

    /* Processing done, insert it */
    LdrpInsertDataTableEntry(LdrpImageEntry);
    LdrpInsertModuleToIndex(LdrpImageEntry, LdrpAppHeaders);

    Status = LdrpProcessMappedModule(LdrpNtDllDataTableEntry, EmptyLoadContextFlags, TRUE);
    if (!NT_SUCCESS(Status))
        return Status;

    LdrpNtDllDataTableEntry->EntryProcessed = TRUE;
    LdrpNtDllDataTableEntry->DdagNode->State = LdrModulesReadyToRun;

    Status = LdrpInitParallelLoadingSupport();
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("LDR: Parallel loader initialization for process " LDR_HEXPTR_FMT " failed with status 0x%08lX\n",
                Teb->ClientId.UniqueProcess, Status);
        return Status;
    }

    LdrpDrainWorkQueue(LoadComplete);

    /* Check if relocation is needed */
    if (Peb->ImageBaseAddress != (PVOID)LdrpAppHeaders->OptionalHeader.ImageBase && ImageHasNativeCode)
    {
        PVOID ViewBase = Peb->ImageBaseAddress;

        DPRINT1("LDR: Performing EXE relocation\n");

        /* Do the relocation */
        Status = LdrpProtectAndRelocateImage(ViewBase,
            NULL,
            STATUS_SUCCESS,
            STATUS_CONFLICTING_ADDRESSES,
            STATUS_INVALID_IMAGE_FORMAT);

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("LdrpProtectAndRelocateImage() failed [0x%08lX]\n", Status);
            return Status;
        }

        /* Check if a start context was provided */
        if (Context)
        {
            DPRINT1("WARNING: Relocated EXE Context");
            UNIMPLEMENTED; // We should support this
            return STATUS_INVALID_IMAGE_FORMAT;
        }
    }

    LdrpImageEntry->DllBase = Peb->ImageBaseAddress;

    Status = LdrpProcessMappedModule(LdrpImageEntry, EmptyLoadContextFlags, !UseWOW64);
    if (!NT_SUCCESS(Status))
        return Status;

    LdrpImageEntry->EntryProcessed = TRUE;

    RtlpCheckListEntry(&PebLdr.InLoadOrderModuleList);
    RtlpCheckListEntry(&PebLdr.InMemoryOrderModuleList);
    RtlpCheckListEntry(&PebLdr.InInitializationOrderModuleList);

    /* Let the world know */
    if (LdrpDebugFlags.LogTrace)
    {
        DPRINT1("LDR: NEW PROCESS\n");
        DPRINT1("     Image Path: %wZ (%wZ)\n", &LdrpImageEntry->FullDllName, &LdrpImageEntry->BaseDllName);
        DPRINT1("     Current Directory: %wZ\n", &CurrentDirectory);
        DPRINT1("     Search Path: %wZ\n", &LdrpDefaultPath);
    }

    /* Initialize Wine's active context implementation for the current process */
    actctx_init(&OldShimData);

    /* Set the current directory */
    Status = RtlSetCurrentDirectory_U(&CurrentDirectory);
    if (!NT_SUCCESS(Status))
    {
        /* We failed, check if we should free it */
        if (FreeCurDir) RtlFreeUnicodeString(&CurrentDirectory);

        /* Set it to the NT Root */
        CurrentDirectory = NtSystemRoot;
        RtlSetCurrentDirectory_U(&CurrentDirectory);
    }
    else
    {
        /* We're done with it, free it */
        if (FreeCurDir) RtlFreeUnicodeString(&CurrentDirectory);
    }

    /* Check if we should look for a .local file */
    if (ProcessParameters && !(ProcessParameters->Flags & RTL_USER_PROCESS_PARAMETERS_LOCAL_DLL_PATH))
    {
        LdrpInitializeDotLocalSupport(ProcessParameters);
    }

    /* Check if the Application Verifier was enabled */
    if (Peb->NtGlobalFlag & FLG_APPLICATION_VERIFIER)
    {
        Status = AVrfInitializeVerifier();
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("LDR: AVrfInitializeVerifier failed (ntstatus 0x%08lX)\n", Status);
            return Status;
        }
    }

    LdrpDropLastInProgressCount();

    PLDR_DATA_TABLE_ENTRY MscoreeEntry = NULL;

    if (UseCOR)
    {
        if (!NT_SUCCESS(Status = LdrpCorInitialize(&MscoreeEntry)))
            return Status;

        if (!ImageHasNativeCode)
        {
            if (!NT_SUCCESS(Status = LdrpCorValidateImage(Peb->ImageBaseAddress, LdrpImageEntry->FullDllName.Buffer)))
            {
                DPRINT1("LDR: LdrpCorValidateImage failed [0x%08lX]\n", Status);
                return Status;
            }
        }

        if (Teb->InitialThread)
            LdrpCorReplaceStartContext(Context);
    }

    /* Initialize TLS */
    Status = LdrpInitializeTls();
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("LDR: LdrpProcessInitialization failed to initialize TLS slots; status 0x%08lX\n",
            Status);
        return Status;
    }

    /* Initialize COR subgraph */
    if (MscoreeEntry)
    {
        BOOLEAN CorHasInitializing = 0;

        Status = LdrpInitializeGraphRecurse(MscoreeEntry->DdagNode, NULL, &CorHasInitializing);

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("LDR: DllMain of MSCOREE (or its dependents) failed with status 0x%08lX\n", Status);
            return Status;
        }

        MscoreeEntry = NULL;
    }

    if (LdrpAppHeaders->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI ||
        LdrpAppHeaders->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI)
    {
        PVOID Kernel32BaseAddress;
        PVOID FunctionAddress;

        Status = LdrLoadDll(NULL, NULL, const_cast<PUNICODE_STRING>(&LdrpKernel32DllName), &Kernel32BaseAddress);

        if (!NT_SUCCESS(Status))
        {
            if (LdrpDebugFlags.LogWarning)
                DPRINT1("LDR: Unable to load %wZ, Status=0x%08lX\n", &LdrpKernel32DllName, Status);
            return Status;
        }

        Status = LdrGetProcedureAddress(Kernel32BaseAddress,
                                        &BaseProcessInitPostImportName,
                                        0,
                                        &FunctionAddress);

        if (!NT_SUCCESS(Status))
        {
            if (LdrpDebugFlags.LogWarning)
                DPRINT1("LDR: Unable to find post-import process init function, Status=0x%08lX\n", &LdrpKernel32DllName, Status);
            return Status;
        }

        Kernel32ProcessInitPostImportFunction = static_cast<decltype(Kernel32ProcessInitPostImportFunction)>(FunctionAddress);

        Status = LdrGetProcedureAddress(Kernel32BaseAddress,
                                        &BaseQueryModuleDataName,
                                        0,
                                        &FunctionAddress);

        if (!NT_SUCCESS(Status))
        {
            if (LdrpDebugFlags.LogWarning)
                DPRINT1("LDR: Unable to find BaseQueryModuleData, Status=0x%08lX\n", &LdrpKernel32DllName, Status);
            return Status;
        }

        Kernel32BaseQueryModuleData = static_cast<decltype(Kernel32BaseQueryModuleData)>(FunctionAddress);
    }

    LdrpDrainWorkQueue(LoadComplete);

    LdrInitState = 1;
    LdrpImageEntry->DdagNode->State = LdrModulesMapped;

    LdrpApplyProcessPermanentNoExecuteMitigationPolicy();

    /* Walk the IAT and load all the DLLs */
    if (!ImageHasNativeCode)
    {
        Status = LdrpCorProcessImports(LdrpImageEntry);
    }
    else
    {
        LdrpImageEntry->LoadContext->Flags.ProcessImage = TRUE;

        Status = LdrpMapAndSnapDependency(LdrpImageEntry->LoadContext);

        LdrpDrainWorkQueue(WorkComplete);

        Status = *LdrpImageEntry->LoadContext->LoadStatus;

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("LDR: Mapping/snapping process dependency tree failed; status 0x%08lX\n", Status);
            return Status;
        }

        ASSERT(LdrpImageEntry->DdagNode->State >= LdrModulesSnapped);

        Status = LdrpPrepareModuleForExecution(LdrpImageEntry, LdrpImageEntry->LoadContext->LoadStatus);
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("LDR: Dependency initialization failed; status 0x%08lX\n", Status);
        return Status;
    }

    /* Lock the DLLs */
    ListHead = &Peb->Ldr->InLoadOrderModuleList;
    NextEntry = ListHead->Flink;
    while (ListHead != NextEntry)
    {
        PLDR_DATA_TABLE_ENTRY LdrEntry = CONTAINING_RECORD(NextEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

        LDRP_ASSERT_MODULE_ENTRY(LdrEntry);
        LDRP_ASSERT_DDAG_NODE(LdrEntry->DdagNode);

        LdrEntry->DdagNode->LoadCount = LDR_LOADCOUNT_MAX;
        NextEntry = NextEntry->Flink;
    }

    ASSERT(LdrpImageEntry->DdagNode->State == LdrModulesReadyToInit);

    /* FIXME Mark the DLL Ranges for Stack Traces later */

    LdrInitState = 2;

    /* Notify the debugger now */
    if (Peb->BeingDebugged)
        LdrpDoDebuggerBreak();

    LdrpDropLastInProgressCount();

    LdrpApplyProcessNoExecuteMitigationPolicy();

    /* Validate the Image for MP Usage */
    Status = LdrpValidateImageForMp(LdrpImageEntry);
    if (!NT_SUCCESS(Status))
        return Status;

    LdrpInitializeProcessCSD();


    LdrpProcessInitContextRecord = Context;

    LdrpDrainWorkQueue(LoadComplete);

    {
        RtlEnterCriticalSection(&LdrpLoaderLock);

        BOOLEAN HasInitializingNode = FALSE;
        Status = LdrpInitializeGraphRecurse(LdrpImageEntry->DdagNode, LdrpImageEntry->LoadContext->LoadStatus, &HasInitializingNode);

        RtlLeaveCriticalSection(&LdrpLoaderLock);
    }

    LdrpFreeLoadContextOfNode(LdrpImageEntry->DdagNode, LdrpImageEntry->LoadContext->LoadStatus);
    LdrpDropLastInProgressCount();

    LdrpProcessInitContextRecord = NULL;


    /* Check if we had Shim Data */
    if (OldShimData)
    {
        /* Load the Shim Engine */
        Peb->AppCompatInfo = NULL;
        LdrpLoadShimEngine((PWSTR)OldShimData, &ImagePathName, OldShimData);
    }
    else
    {
        /* Check for Application Compatibility Goo */
        //LdrQueryApplicationCompatibilityGoo(hKey);
        DPRINT("Querying app compat hacks is missing!\n");
    }

    /* Notify Shim Engine */
    if (g_ShimsEnabled)
    {
        void(NTAPI *SE_InstallAfterInit)(PUNICODE_STRING, PVOID);
        SE_InstallAfterInit = static_cast<decltype(SE_InstallAfterInit)>(RtlDecodeSystemPointer(g_pfnSE_InstallAfterInit));
        SE_InstallAfterInit(&ImagePathName, OldShimData);
    }

    /* Check if we have a user-defined Post Process Routine */
    if (NT_SUCCESS(Status) && Peb->PostProcessInitRoutine)
    {
        /* Call it */
        Peb->PostProcessInitRoutine();
    }

    /* Close the key if we have one opened */
    if (OptionsKey) NtClose(OptionsKey);

    /* Return status */
    return Status;
    LDR_FUNC_END_RETURN
}

void
NTAPI
LdrpInitFailure(NTSTATUS Status)
{
    LDR_FUNC_VOID(Status)
    ULONG Response;
    PPEB Peb = NtCurrentPeb();

    /* Print a debug message */
    DPRINT1("LDR: Process initialization failure for %wZ; NTSTATUS = 0x%08lX\n",
            &Peb->ProcessParameters->ImagePathName, Status);

    // todo: remove this
    if (LdrpDebugFlags.BreakInDebugger || TRUE)
        __debugbreak();

    /* Raise a hard error */
    if (!LdrpFatalHardErrorCount)
    {
        NtRaiseHardError(STATUS_APP_INIT_FAILURE, 1, 0, (PULONG_PTR)&Status, OptionOk, &Response);
    }
    LDR_FUNC_END_VOID
}

NTSTATUS
FASTCALL
LdrpTouchThreadStack(ULONG_PTR MinimumStackCommit)
{
    LDR_FUNC(NTSTATUS, MinimumStackCommit)
    MEMORY_BASIC_INFORMATION Info = {0};
    SIZE_T InfoLength = 0;
    const PTEB Teb = NtCurrentTeb();

    NTSTATUS Status = NtQueryVirtualMemory(NtCurrentProcess(), Teb->NtTib.StackLimit, MemoryBasicInformation, &Info, sizeof(MEMORY_BASIC_INFORMATION), &InfoLength);

    if (NT_SUCCESS(Status))
    {
        const ULONG_PTR OffsetBase = (ULONG_PTR)Info.AllocationBase + 0x3000;
        ULONG_PTR Ptr = (ULONG_PTR)Teb->NtTib.StackBase - PAGE_SIZE;
        ULONG_PTR LowerBound = Ptr - MinimumStackCommit;

        if (Ptr <= MinimumStackCommit || LowerBound <= OffsetBase)
            LowerBound = OffsetBase;

        while (Ptr >= LowerBound)
        {
            [&Ptr]() -> void {
                __try
                {
                    /* todo: make sure the read is not optimized away even in Release */
                    volatile BYTE Dummy = *(volatile BYTE*)Ptr;

                    // ReSharper disable once CppIdenticalOperandsInBinaryExpression, CppAssignedValueIsNeverUsed
                    Dummy ^= Dummy;
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    DPRINT1("Failed to allocate thread stack memory: 0x%08lX\n", GetExceptionCode());
                    _SEH2_YIELD(return STATUS_NO_MEMORY);
                }
            }();

            Ptr -= PAGE_SIZE;
        }

        Status = STATUS_SUCCESS;
    }

    return Status;
    LDR_FUNC_END_RETURN
}

void
FASTCALL
LdrpProcessInitializationComplete()
{
    LDR_FUNC_VOID_MANUAL()
    InterlockedIncrement(&LdrpProcessInitialized);

    if (LdrpInitCompleteEvent)
        NtSetEvent(LdrpInitCompleteEvent, NULL);
    LDR_FUNC_END_VOID
}

void
NTAPI
LdrpDoDebuggerBreak()
{
    LDR_FUNC_VOID_MANUAL()
    NTSTATUS Status;
    BOOLEAN HideValue;

    Status = NtQueryInformationThread(NtCurrentThread(), ThreadHideFromDebugger, &HideValue, sizeof(BOOLEAN), NULL);
    if (NT_SUCCESS(Status) && !HideValue)
        __debugbreak();
    LDR_FUNC_END_VOID
}

BOOLEAN
FASTCALL
LdrpWaitForInit()
{
    LDR_FUNC_MANUAL(BOOLEAN)
    /* 30 milliseconds */
    LARGE_INTEGER Timeout = {
        .QuadPart = Int32x32To64(30, -10000)
    };

    if (LdrpInitCompleteEvent)
    {
        const NTSTATUS Status = NtWaitForSingleObject(LdrpInitCompleteEvent, FALSE, NULL);

        if (NT_SUCCESS(Status))
            return TRUE;

        DPRINT1("NtWaitForSingleObject failed with status 0x%08lX, fallback to delay loop\n", Status);
    }

    /* Make sure the status hasn't changed */
    while (LdrpProcessInitialized == 1)
    {
        /* Do the wait */
        const NTSTATUS Status = NtDelayExecution(FALSE, &Timeout);

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Delaying execution failed with status 0x%08lX\n", Status);
            if (LdrpDebugFlags.BreakInDebugger)
                __debugbreak();
        }
    }

    return FALSE;
    LDR_FUNC_END_IMPL(return FALSE)
}

NTSTATUS
FASTCALL
LdrpInitializeState0(PCONTEXT Context, PVOID SystemDllBaseAddress)
{
    LDR_FUNC(NTSTATUS, Cotnext, SystemDllBaseAddress)
    NTSTATUS Status;
    const PTEB Teb = NtCurrentTeb();
    const PPEB Peb = Teb->ProcessEnvironmentBlock;

    RtlpInitializeKeyedEvent();
    NtCreateEvent(&LdrpInitCompleteEvent, EVENT_ALL_ACCESS, NULL, NotificationEvent, FALSE);

    Teb->RanProcessInit = TRUE;
    Peb->LoaderLock = &LdrpLoaderLock;
    LdrInitState = 0;
    PspSetPebProcessInitializingFlagAssert(Peb);

    /* Protect with SEH */
    __try
    {
        /* Initialize the Process */
        Status = LdrpInitializeProcess(Context, SystemDllBaseAddress);
    }
    __except(LdrpDebugExceptionFilter(GetExceptionCode(), GetExceptionInformation()))
    {
        /* Fail with the SEH error */
        Status = GetExceptionCode();
    }

    /* Check if init worked */
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Process initialization failed with status 0x%08lX\n", Status);
        return Status;
    }

    /* Check for success and if MinimumStackCommit was requested */
    if (Peb->MinimumStackCommit)
    {
        /* Enforce the limit */
        Status = LdrpTouchThreadStack(Peb->MinimumStackCommit);
    }

    LdrInitState = 3;
    PspClearPebProcessInitializingFlagAssert(Peb);

    if (!NT_SUCCESS(Status))
        return Status;

    if (!UseWOW64 || LdrpProcessInitialized == 1)
        LdrpProcessInitializationComplete();

    return Status;
    LDR_FUNC_END_RETURN
}

NTSTATUS
FASTCALL
LdrpInitializeStateF(PCONTEXT Context)
{
    LDR_FUNC(NTSTATUS, Context)
    /* This is a new thread initializing */
    LdrpInitializeThread(Context);

    return STATUS_SUCCESS;
    LDR_FUNC_END_RETURN
}

NTSTATUS
FASTCALL
LdrpInitializeStateE(PCONTEXT Context)
{
    LDR_FUNC(NTSTATUS, Context)
    const PTEB Teb = NtCurrentTeb();
    const PPEB Peb = Teb->ProcessEnvironmentBlock;

    /* LdrpProcessInitialized is 2, Peb->InheritedAddressSpace != NULL */

    if (Peb->InheritedAddressSpace)
    {
        ASSERT(Peb->ProcessHeap);

        // Fork stuff

        Peb->InheritedAddressSpace = FALSE;
        if (Peb->BeingDebugged)
            LdrpDoDebuggerBreak();
    }

    LdrpProcessInitializationComplete();

    return LdrpInitializeStateF(Context);
    LDR_FUNC_END_RETURN
}

NTSTATUS
FASTCALL
LdrpInitializeStateA(PCONTEXT Context, PVOID SystemDllBaseAddress)
{
    LDR_FUNC(NTSTATUS, Context, SystemDllBaseAddress)
    const PTEB Teb = NtCurrentTeb();
    const PPEB Peb = Teb->ProcessEnvironmentBlock;

    do
    {
        LONG ProcessInitialized = InterlockedCompareExchange(&LdrpProcessInitialized, 1, 0);

        /* Now check if the process is already being initialized */
        if (ProcessInitialized == 1 && !Teb->LoaderWorker)
            LdrpWaitForInit();

        do
        {
            ASSERT(!ProcessInitialized || ProcessInitialized == 2);

            if (!ProcessInitialized)
                return LdrpInitializeState0(Context, SystemDllBaseAddress);

            ASSERT(ProcessInitialized == 2);

            /* Process is already initialized... is this a fork() ? */
            if (!Peb->InheritedAddressSpace)
                return LdrpInitializeStateF(Context);

            /* Handle the fork() */
            LdrpInitCompleteEvent = NULL;

            if (InterlockedCompareExchange(&LdrpProcessInitialized, 1, 2) == 2)
                return LdrpInitializeStateE(Context);
        }
        while (LdrpWaitForInit());
    }
    while (TRUE);
    LDR_FUNC_END_RETURN
}

void
NTAPI
LdrpInitialize(PCONTEXT Context, PVOID SystemDllBaseAddress)
{
    LDR_FUNC_VOID(Context, SystemDllBaseAddress)
    NTSTATUS Status;
    const PTEB Teb = NtCurrentTeb();

    DPRINT("LdrpInitialize() " LDR_HEXPTR_FMT "/" LDR_HEXPTR_FMT "\n", Teb->RealClientId.UniqueProcess, Teb->RealClientId.UniqueThread);

    Status = LdrpInitializeStateA(Context, SystemDllBaseAddress);

    if (NT_SUCCESS(Status))
    {
        /* All done, test alert the thread */
        if (!NtCurrentTeb()->LoaderWorker)
            NtTestAlert();
    }
    else
    {
        /* Fail */
        LdrpInitFailure(Status);
        RtlRaiseStatus(Status);
    }
    LDR_FUNC_END_VOID
}

void
FASTCALL
_cpu_features_init()
{
    LDR_FUNC_VOID_MANUAL()
#ifdef _WIN64
    /* Set the SList header usage */
    RtlpUse16ByteSLists = SharedUserData->ProcessorFeatures[PF_COMPARE_EXCHANGE128];
#endif /* _WIN64 */
    LDR_FUNC_END_VOID
}

EXTERN_C
void
NTAPI
LdrpInit(PCONTEXT Context, PVOID SystemArgument1, PVOID SystemArgument2)
{
    LDR_FUNC_VOID(Context, SystemArgument1, SystemArgument2)
    ASSERT(NtCurrentTeb()->DeallocationStack);

    if (!SecurityCookieInitialized)
        InitSecurityCookie();

    _cpu_features_init();

    if (!NtCurrentTeb()->SkipLoaderInit)
        LdrpInitialize(Context, SystemArgument1);
    LDR_FUNC_END_VOID
}

/* EOF */
