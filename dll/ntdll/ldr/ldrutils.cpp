/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS NT User-Mode Library
 * FILE:            dll/ntdll/ldr/ldrutils.c
 * PURPOSE:         Internal Loader Utility Functions
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ldrp.h>

EXTERN_C
{

#include <ndk/exfuncs.h>
#include <reactos/ldrp.h>

}

/* GLOBALS *******************************************************************/

UNICODE_STRING SlashSystem32SlashString = RTL_CONSTANT_STRING(L"\\System32\\");

BOOLEAN g_ShimsEnabled;
PVOID g_pShimEngineModule;
PVOID g_pfnSE_DllLoaded;
PVOID g_pfnSE_DllUnloaded;
PVOID g_pfnSE_InstallBeforeInit;
PVOID g_pfnSE_InstallAfterInit;
PVOID g_pfnSE_ProcessDying;
PVOID g_pfnSE_LdrResolveDllName;
ULONG LdrpWorkInProgress = 0;

LDRP_DEBUG_FLAGS LdrpDebugFlags = {0};

PVOID LdrpHeap;

HANDLE LdrpLoadCompleteEvent, LdrpWorkCompleteEvent;

/* FUNCTIONS *****************************************************************/

void* operator new(size_t sz)
{
    return RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, sz);
}

void operator delete(void* ptr) noexcept
{
    RtlFreeHeap(RtlGetProcessHeap(), 0, ptr);
}

void* operator new[](size_t sz)
{
    return RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, sz);
}

void operator delete[](void* ptr) noexcept
{
    RtlFreeHeap(RtlGetProcessHeap(), 0, ptr);
}

EXTERN_C
BOOLEAN
NTAPI
LdrpCallInitRoutine(IN PDLL_INIT_ROUTINE EntryPoint,
                    IN PVOID BaseAddress,
                    IN ULONG Reason,
                    IN PVOID Context)
{
    /* Call the entry */
    return EntryPoint(BaseAddress, Reason, static_cast<PCONTEXT>(Context));
}

NTSTATUS
NTAPI
LdrpCodeAuthzCheckDllAllowed(IN PUNICODE_STRING FullName,
                             IN HANDLE DllHandle)
{
    /* Not implemented */
    return STATUS_SUCCESS;
}

LDRP_LOAD_CONTEXT_FLAGS
NTAPI
LdrpDllCharacteristicsToLoadFlags(ULONG DllCharacteristics)
{
    LDRP_LOAD_CONTEXT_FLAGS Result = {0};
    if (DllCharacteristics & IMAGE_FILE_EXECUTABLE_IMAGE)
        Result.ExecutableImage = TRUE;
    if (DllCharacteristics & IMAGE_FILE_SYSTEM)
        Result.SystemImage = TRUE;
    if (DllCharacteristics & IMAGE_DLLCHARACTERISTICS_APPCONTAINER)
        Result.AppContainerImage = TRUE;
    return Result;
}

NTSTATUS
NTAPI
LdrpBuildSystem32FileName(IN PLDRP_UNICODE_STRING_BUNDLE DestinationString,
                          IN PUNICODE_STRING FileName OPTIONAL)
{
    ASSERT(DestinationString);
    DestinationString->String.Length = 0;

    UNICODE_STRING NtSystemRoot;
    RtlInitUnicodeString(&NtSystemRoot, SharedUserData->NtSystemRoot);
    ASSERT(NT_SUCCESS(LdrpAppendUnicodeStringToFilenameBuffer(DestinationString, &NtSystemRoot)));
    ASSERT(NT_SUCCESS(LdrpAppendUnicodeStringToFilenameBuffer(DestinationString, &SlashSystem32SlashString)));

    if (!FileName)
        return STATUS_SUCCESS;
    return LdrpAppendUnicodeStringToFilenameBuffer(DestinationString, FileName);
}

ULONG32
NTAPI
LdrpHashUnicodeString(IN PUNICODE_STRING NameString)
{
    ASSERT(NameString);
    ASSERT(NameString->Buffer);

    ULONG Result = 0;
    if (!NT_SUCCESS(RtlHashUnicodeString(NameString, TRUE, HASH_STRING_ALGORITHM_DEFAULT, &Result)))
    {
        Result = MINLONG;
    }
    return Result;
}

BOOLEAN
NTAPI
LdrpIsBaseNameOnly(IN PUNICODE_STRING DllName)
{
    WCHAR *p = DllName->Buffer + (DllName->Length / sizeof(WCHAR)) - 1;
    while (p >= DllName->Buffer)
    {
        const WCHAR c = *p--;
        if (c == L'\\' || c == L'/')
            return FALSE;
    }
    return TRUE;
}

PVOID
NTAPI
LdrpHeapAlloc(IN ULONG Flags OPTIONAL,
              IN SIZE_T Size)
{
    return RtlAllocateHeap(LdrpHeap,
                           Flags,
                           Size);
}

BOOLEAN
NTAPI
LdrpHeapFree(IN ULONG Flags OPTIONAL,
             IN PVOID BaseAddress)
{
    return RtlFreeHeap(LdrpHeap, Flags, BaseAddress);
}

PVOID
NTAPI
LdrpHeapReAlloc(IN ULONG Flags OPTIONAL,
                IN PVOID Ptr,
                IN SIZE_T Size)
{
    return RtlReAllocateHeap(LdrpHeap,
                             Flags,
                             Ptr,
                             Size);
}

PSINGLE_LIST_ENTRY
NTAPI
LdrpFindPreviousAnyEntry(LDRP_CSLIST List, PSINGLE_LIST_ENTRY TargetEntry)
{
    // Search list for the entry before TargetEntry.
    //
    // That is, iterate List and compare each link with TargetEntry
    // which will, once equal, be precisely one link after the one we actually need.

    const PSINGLE_LIST_ENTRY ListHead = List.Tail;

    ASSERT(ListHead);

    PSINGLE_LIST_ENTRY FoundLink = ListHead;

    for (PSINGLE_LIST_ENTRY ListEntry = ListHead ? ListHead->Next : NULL;
        ListEntry;
        ListEntry = ListEntry != ListHead ? ListEntry->Next : NULL)
    {
        if (ListEntry == TargetEntry)
            break;

        // Store the old entry.
        FoundLink = ListEntry;

        ASSERT(FoundLink != ListHead);
    }

    ASSERT(FoundLink->Next == TargetEntry);

    // Return what is needed.
    return FoundLink;
}

void
NTAPI
LdrpRemoveDependencyEntry(PLDRP_CSLIST List, PSINGLE_LIST_ENTRY TargetEntry)
{
    // Find the previous entry for the TargetEntry to be able to remove the latter.
    // That's the unfortunate disadvantage of using singly-linked lists.
    const PSINGLE_LIST_ENTRY FoundLink = LdrpFindPreviousAnyEntry(*List, TargetEntry);

    // Verify that we have found the right thing.
    ASSERT(FoundLink);
    ASSERT(FoundLink->Next == TargetEntry);

    // Drop the TargetEntry by connecting the incoming and outgoing links.
    FoundLink->Next = TargetEntry->Next;

    // Did we just remove the List's tail?
    if (List->Tail == TargetEntry)
    {
        // Set the new tail of the list
        // Collapse the whole list to NULL iff empty list, or, technically, list with a single element just removed
        List->Tail = FoundLink == TargetEntry ? NULL : FoundLink;
    }
}

NTSTATUS
NTAPI
LdrpCreateLoaderEvents(void)
{
    NTSTATUS Status = NtCreateEvent(&LdrpLoadCompleteEvent, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, FALSE);

    if (NT_SUCCESS(Status))
        Status = NtCreateEvent(&LdrpWorkCompleteEvent, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, FALSE);

    return Status;
}

NTSTATUS
NTAPI
LdrpInitParallelLoadingSupport(void)
{
    InitializeListHead(&LdrpWorkQueue);
    InitializeListHead(&LdrpRetryQueue);
    RtlInitializeCriticalSection(&LdrpWorkQueueLock);
    return LdrpCreateLoaderEvents();
}

void
NTAPI
LdrpDropLastInProgressCount(void)
{
    DPRINT1("LdrpDropLastInProgressCount(): LdrpWorkInProgress was %lu\n", LdrpWorkInProgress);

    NtCurrentTeb()->LoadOwner = FALSE;
    RtlEnterCriticalSection(&LdrpWorkQueueLock);
    LdrpWorkInProgress = 0;
    RtlLeaveCriticalSection(&LdrpWorkQueueLock);
    NtSetEvent(LdrpLoadCompleteEvent, NULL);
}

LONG
LdrpCompareModuleName(PLDR_DATA_TABLE_ENTRY SearchTarget, PLDR_DATA_TABLE_ENTRY Item)
{
    LDRP_ASSERT_MODULE_ENTRY(SearchTarget);
    LDRP_ASSERT_MODULE_ENTRY(Item);

    const LONG Result = Item->BaseNameHashValue - SearchTarget->BaseNameHashValue;

    if (Result)
        return Result;

    return RtlCompareUnicodeString(&Item->FullDllName, &SearchTarget->FullDllName, TRUE);
}

BOOLEAN
NTAPI
LdrpCheckForRetryLoading(IN PLDRP_LOAD_CONTEXT LoadContext, IN BOOLEAN InsertIntoRetryQueue)
{
    LDRP_ASSERT_LOAD_CONTEXT(LoadContext);

    BOOLEAN Inserted = FALSE;

    if (!LoadContext->Flags.InRetryQueue && !LoadContext->Flags.RetryDependentLoadWhenMissing)
    {
        const PLDR_DATA_TABLE_ENTRY Entry = LoadContext->Module;
        const PLIST_ENTRY ListHead = &LdrpRetryQueue;
        PLDRP_LOAD_CONTEXT Found = NULL;

        LDRP_ASSERT_MODULE_ENTRY(Entry);

        RtlEnterCriticalSection(&LdrpWorkQueueLock);

        for (PLIST_ENTRY It = ListHead->Flink; It != ListHead; It = It->Flink)
        {
            /* Get the current entry */
            const PLDRP_LOAD_CONTEXT Current = CONTAINING_RECORD(It, LDRP_LOAD_CONTEXT, QueueListEntry);

            LDRP_ASSERT_LOAD_CONTEXT(Current);

            if (LdrpCompareModuleName(Entry, Current->Module))
            {
                Found = Current;
                break;
            }
        }

        if (Found || InsertIntoRetryQueue)
        {
            Inserted = TRUE;
            LoadContext->Flags.InRetryQueue = TRUE;
            InsertTailList(ListHead, &LoadContext->QueueListEntry);
        }

        RtlLeaveCriticalSection(&LdrpWorkQueueLock);
    }

    return Inserted;
}

NTSTATUS
NTAPI
LdrpAppCompatRedirect(PLDRP_LOAD_CONTEXT LoadContext, PUNICODE_STRING FullDosPath, PUNICODE_STRING BaseDllName, PLDRP_UNICODE_STRING_BUNDLE NtPath, NTSTATUS Status)
{
    LDRP_ASSERT_LOAD_CONTEXT(LoadContext);

    if (!g_ShimsEnabled || !g_pfnSE_LdrResolveDllName)
        return Status;

    UNICODE_STRING String;

    void (NTAPI * SE_LdrResolveDllName)(PUNICODE_STRING, PBOOLEAN, PUNICODE_STRING) =
        static_cast<decltype(SE_LdrResolveDllName)>(RtlDecodeSystemPointer(g_pfnSE_LdrResolveDllName));

    const PUNICODE_STRING DllName = NT_SUCCESS(Status) ? &NtPath->String : &LoadContext->DllName;
    BOOLEAN Redirected = FALSE;

    SE_LdrResolveDllName(&String, &Redirected, DllName);

    if (Redirected)
    {
        LdrpFreeUnicodeString(FullDosPath);
        RtlEraseUnicodeString(&NtPath->String);
        Status = LdrpResolveDllName(&String, NtPath, BaseDllName, FullDosPath, LoadContext->Flags);
    }

    return Status;
}

LONG
NTAPI
LdrpDebugExceptionFilter(unsigned int code, PEXCEPTION_POINTERS ep)
{
#ifndef NDEBUG
    if (LdrpDebugFlags.BreakInDebugger)
        __debugbreak();
#endif

    return EXCEPTION_EXECUTE_HANDLER;
}

void
NTAPI
LdrpInsertModuleToIndexLockHeld(IN PLDR_DATA_TABLE_ENTRY Module, IN PIMAGE_NT_HEADERS NtHeader)
{
    Module->TimeDateStamp = NtHeader->FileHeader.TimeDateStamp;
    Module->SizeOfImage = NtHeader->OptionalHeader.SizeOfImage;

    // todo: insert Module into Red-Black tree

    Module->InIndexes = TRUE;
}

void
NTAPI
LdrpInsertModuleToIndex(IN PLDR_DATA_TABLE_ENTRY Module, IN PIMAGE_NT_HEADERS NtHeader)
{
    RtlAcquireSRWLockExclusive(&LdrpModuleDatatableLock);
    LdrpInsertModuleToIndexLockHeld(Module, NtHeader);
    RtlReleaseSRWLockExclusive(&LdrpModuleDatatableLock);
}


/* EOF */
