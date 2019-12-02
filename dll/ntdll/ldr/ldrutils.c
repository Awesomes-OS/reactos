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

/* GLOBALS *******************************************************************/

UNICODE_STRING SlashSystem32SlashString = RTL_CONSTANT_STRING(L"\\System32\\");

BOOLEAN g_ShimsEnabled;
PVOID g_pShimEngineModule;
PVOID g_pfnSE_DllLoaded;
PVOID g_pfnSE_DllUnloaded;
PVOID g_pfnSE_InstallBeforeInit;
PVOID g_pfnSE_InstallAfterInit;
PVOID g_pfnSE_ProcessDying;

LDRP_DEBUG_FLAGS LdrpDebugFlags = {0};

PVOID LdrpHeap;

/* FUNCTIONS *****************************************************************/


BOOLEAN
NTAPI
LdrpCallInitRoutine(IN PDLL_INIT_ROUTINE EntryPoint,
                    IN PVOID BaseAddress,
                    IN ULONG Reason,
                    IN PVOID Context)
{
    /* Call the entry */
    return EntryPoint(BaseAddress, Reason, Context);
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
        ListEntry = (ListEntry != ListHead) ? ListEntry->Next : NULL)
    {
        if (ListEntry == TargetEntry)
            break;

        // Store the old entry.
        FoundLink = ListEntry;
    }

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
        List->Tail = (FoundLink == TargetEntry) ? NULL : FoundLink;
    }
}

/* EOF */
