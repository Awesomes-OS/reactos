/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/rtl/loader.c
 * PURPOSE:         Shared RTL PE image loader functions (w/o allocation)
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

LDRP_HEAPALLOC_FUNC *LdrpHeapAllocProc = NULL;
LDRP_HEAPFREE_FUNC *LdrpHeapFreeProc = NULL;
LDRP_HEAPREALLOC_FUNC *LdrpHeapReAllocProc = NULL;

PVOID
NTAPI
LdrpAllocateHeap(IN ULONG Flags OPTIONAL,
                 IN SIZE_T Size)
{
    if (!LdrpHeapAllocProc)
    {
        DPRINT1("LdrpHeapAllocProc is NULL");
        return NULL;
    }

    return LdrpHeapAllocProc(Flags, Size);
}

BOOLEAN
NTAPI
LdrpFreeHeap(IN ULONG Flags OPTIONAL,
             IN PVOID BaseAddress)
{
    if (!LdrpHeapFreeProc)
    {
        DPRINT1("LdrpHeapFreeProc is NULL");
        return FALSE;
    }

    return LdrpHeapFreeProc(Flags, BaseAddress);
}

PVOID
NTAPI
LdrpReAllocateHeap(IN ULONG Flags OPTIONAL,
                   IN PVOID Ptr,
                   IN SIZE_T Size)
{
    if (!LdrpHeapReAllocProc)
    {
        DPRINT1("LdrpHeapReAllocProc is NULL");
        return NULL;
    }

    return LdrpHeapReAllocProc(Flags, Ptr, Size);
}

PVOID
NTAPI
LdrpFetchAddressOfEntryPoint(IN PVOID ImageBase)
{
    PIMAGE_NT_HEADERS NtHeaders;
    ULONG_PTR EntryPoint = 0;

    /* Get entry point offset from NT headers */
    NtHeaders = RtlImageNtHeader(ImageBase);
    if (NtHeaders)
    {
        /* Add image base */
        EntryPoint = NtHeaders->OptionalHeader.AddressOfEntryPoint;
        if (EntryPoint)
            EntryPoint += (ULONG_PTR) ImageBase;
    }

    /* Return calculated pointer (or zero in case of failure) */
    return (PVOID) EntryPoint;
}

VOID
NTAPI
LdrpAssignDataTableEntryBaseAddress(IN PLDR_DATA_TABLE_ENTRY LdrEntry,
                                    IN PVOID BaseAddress)
{
    ASSERT(LdrEntry);
    ASSERT(BaseAddress);

    /* Make sure the header is valid */
    PIMAGE_NT_HEADERS NtHeader = RtlImageNtHeader(BaseAddress);

    if (NtHeader)
    {
        LdrEntry->DllBase = BaseAddress;
        LdrEntry->SizeOfImage = NtHeader->OptionalHeader.SizeOfImage;
        LdrEntry->TimeDateStamp = NtHeader->FileHeader.TimeDateStamp;
    }
}

NTSTATUS
NTAPI
LdrpAllocateUnicodeString(IN OUT PUNICODE_STRING StringOut,
                          IN ULONG Length)
{
    /* Sanity checks */
    ASSERT(StringOut);

    Length += sizeof(WCHAR);
    if (Length > UNICODE_STRING_MAX_BYTES)
        return STATUS_NAME_TOO_LONG;

    /* Assume failure */
    StringOut->Buffer = NULL;
    StringOut->Length = 0;
    StringOut->MaximumLength = 0;

    /* Make sure it's not mis-aligned */
    if (Length & 1u)
    {
        /* Fail */
        return STATUS_INVALID_PARAMETER;
    }

    /* Allocate the string*/
    StringOut->Buffer = LdrpAllocateHeap(0, Length);
    if (!StringOut->Buffer)
    {
        /* Fail */
        return STATUS_NO_MEMORY;
    }

    /* Null-terminate it */
    StringOut->Buffer[0] = UNICODE_NULL;

    StringOut->MaximumLength = Length;

    /* Return success */
    return STATUS_SUCCESS;
}

#define PTR_ADD_OFFSET(Pointer, Offset) ((PVOID)((ULONG_PTR)(Pointer) + (ULONG_PTR)(Offset)))

VOID
NTAPI
LdrpFreeUnicodeString(IN OUT PUNICODE_STRING StringIn)
{
    ASSERT(StringIn);

    /* If Buffer is not NULL - free it */
    if (StringIn->Buffer)
    {
        if (StringIn->Buffer == PTR_ADD_OFFSET(StringIn, sizeof(UNICODE_STRING)))
        {
            DPRINT1("Freeing buffer allocated on stack");
            __debugbreak();
        }
        else
        {
            LdrpFreeHeap(0, StringIn->Buffer);
        }
    }

    /* Zero it out */
    RtlInitEmptyUnicodeString(StringIn, NULL, 0);
}
