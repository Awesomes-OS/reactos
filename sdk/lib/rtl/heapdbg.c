/*
 * PROJECT:         ReactOS Runtime Library
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            lib/rtl/heapdbg.c
 * PURPOSE:         Heap manager debug heap
 * PROGRAMMERS:     Copyright 2010 Aleksey Bragin
 */

/* INCLUDES ******************************************************************/

#include <rtl.h>
#include <heap.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

volatile BOOLEAN RtlpHeapInvalidBreakPoint = FALSE;
volatile PVOID RtlpHeapInvalidBadAddress = NULL;

/* FUNCTIONS ******************************************************************/

HANDLE NTAPI
RtlDebugCreateHeap(ULONG Flags,
                   PVOID Addr,
                   SIZE_T ReserveSize,
                   SIZE_T CommitSize,
                   PVOID Lock,
                   PRTL_HEAP_PARAMETERS Parameters)
{
    MEMORY_BASIC_INFORMATION MemoryInfo;
    NTSTATUS Status;
    PHEAP Heap;

    /* Validate parameters */
    if (ReserveSize <= HEAP_ENTRY_SIZE)
    {
        DPRINT1("HEAP: Incorrect ReserveSize %x\n", ReserveSize);
        RtlpBreakPointHeap(NULL);
        return NULL;
    }

    if (ReserveSize < CommitSize)
    {
        DPRINT1("HEAP: Incorrect CommitSize %x\n", CommitSize);
        RtlpBreakPointHeap(NULL);
        return NULL;
    }

    if (Flags & HEAP_NO_SERIALIZE && Lock)
    {
        DPRINT1("HEAP: Can't specify Lock routine and have HEAP_NO_SERIALIZE flag set\n");
        RtlpBreakPointHeap(NULL);
        return NULL;
    }

    /* If the address is specified, check it's virtual memory */
    if (Addr)
    {
        Status = ZwQueryVirtualMemory(NtCurrentProcess(),
                                      Addr,
                                      MemoryBasicInformation,
                                      &MemoryInfo,
                                      sizeof(MemoryInfo),
                                      NULL);

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("HEAP: Specified heap base address %p is invalid, Status 0x%08X\n", Addr, Status);
            RtlpBreakPointHeap(NULL);
            return NULL;
        }

        if (MemoryInfo.BaseAddress != Addr)
        {
            DPRINT1("HEAP: Specified heap base address %p is not really a base one %p\n", Addr, MemoryInfo.BaseAddress);
            RtlpBreakPointHeap(NULL);
            return NULL;
        }

        if (MemoryInfo.State == MEM_FREE)
        {
            DPRINT1("HEAP: Specified heap base address %p is free\n", Addr);
            RtlpBreakPointHeap(NULL);
            return NULL;
        }
    }

    /* All validation performed, now call the real routine with skip validation check flag */
    Flags |= HEAP_SKIP_VALIDATION_CHECKS |
             HEAP_TAIL_CHECKING_ENABLED |
             HEAP_FREE_CHECKING_ENABLED;

    Heap = RtlCreateHeap(Flags, Addr, ReserveSize, CommitSize, Lock, Parameters);
    if (!Heap) return NULL;

    // FIXME: Capture stack backtrace

    RtlpValidateHeapHeaders(Heap, TRUE);

    return Heap;
}

BOOLEAN NTAPI
RtlDebugDestroyHeap(HANDLE HeapPtr)
{
    SIZE_T Size = 0;
    PHEAP Heap = (PHEAP)HeapPtr;

    if (Heap == RtlGetCurrentPeb()->ProcessHeap)
    {
        DPRINT1("HEAP: It's forbidden delete process heap!");
        return FALSE;
    }

    if (!RtlpCheckHeapSignature(Heap, "RtlDestroyHeap"))
        return FALSE;

    if (!RtlpValidateHeap(Heap, FALSE)) return FALSE;

    /* Make heap invalid by zeroing its signature */
    Heap->Signature = 0;

    /* Free validate headers copy if it was existing */
    if (Heap->HeaderValidateCopy)
    {
        ZwFreeVirtualMemory(NtCurrentProcess(),
                            &Heap->HeaderValidateCopy,
                            &Size,
                            MEM_RELEASE);
    }

    return TRUE;
}

PVOID NTAPI
RtlDebugAllocateHeap(PVOID HeapPtr,
                     ULONG Flags,
                     SIZE_T Size)
{
    PHEAP Heap = (PHEAP)HeapPtr;
    SIZE_T AllocSize = 1;
    BOOLEAN HeapLocked = FALSE;
    PVOID Result;

    if (Heap->ForceFlags & HEAP_FLAG_PAGE_ALLOCS)
        return RtlpPageHeapAllocate(HeapPtr, Flags, Size);

    if (!RtlpCheckHeapSignature(Heap, "RtlAllocateHeap"))
        return NULL;

    /* Add settable user value flag */
    Flags |= Heap->ForceFlags | HEAP_SETTABLE_USER_VALUE | HEAP_SKIP_VALIDATION_CHECKS;

    /* Calculate size */
    if (Size) AllocSize = Size;
    AllocSize = ((AllocSize + Heap->AlignRound) & Heap->AlignMask) + sizeof(HEAP_ENTRY_EXTRA);

    /* Check if size didn't exceed max one */
    if (AllocSize < Size ||
        AllocSize > Heap->MaximumAllocationSize)
    {
        DPRINT1("HEAP: Too big allocation size %x (max allowed %x)\n", Size, Heap->MaximumAllocationSize);
        return NULL;
    }

    /* Lock the heap ourselves */
    if (!(Flags & HEAP_NO_SERIALIZE))
    {
        RtlEnterHeapLock(Heap->LockVariable, TRUE);
        HeapLocked = TRUE;

        /* Add no serialize flag so that the main routine won't try to acquire the lock again */
        Flags |= HEAP_NO_SERIALIZE;
    }

    /* Validate the heap if necessary */
    RtlpValidateHeap(Heap, FALSE);

    /* Call main routine to do the stuff */
    Result = RtlAllocateHeap(HeapPtr, Flags, Size);

    /* Validate heap headers */
    RtlpValidateHeapHeaders(Heap, TRUE);

    if (Result)
    {
        if (Heap->Flags & HEAP_VALIDATE_ALL_ENABLED)
            RtlpValidateHeap(Heap, FALSE);
    }

    /* Release the lock */
    if (HeapLocked) RtlLeaveHeapLock(Heap->LockVariable);

    return Result;
}

PVOID NTAPI
RtlDebugReAllocateHeap(HANDLE HeapPtr,
                       ULONG Flags,
                       PVOID Ptr,
                       SIZE_T Size)
{
    PHEAP Heap = (PHEAP)HeapPtr;
    SIZE_T AllocSize = 1;
    BOOLEAN HeapLocked = FALSE;
    PVOID Result = NULL;
    PHEAP_ENTRY HeapEntry;

    if (Heap->ForceFlags & HEAP_FLAG_PAGE_ALLOCS)
        return RtlpPageHeapReAllocate(HeapPtr, Flags, Ptr, Size);

    if (!RtlpCheckHeapSignature(Heap, "RtlReAllocateHeap"))
        return NULL;

    /* Add settable user value flag */
    Flags |= Heap->ForceFlags | HEAP_SETTABLE_USER_VALUE | HEAP_SKIP_VALIDATION_CHECKS;

    /* Calculate size */
    if (Size) AllocSize = Size;
    AllocSize = ((AllocSize + Heap->AlignRound) & Heap->AlignMask) + sizeof(HEAP_ENTRY_EXTRA);

    /* Check if size didn't exceed max one */
    if (AllocSize < Size ||
        AllocSize > Heap->MaximumAllocationSize)
    {
        DPRINT1("HEAP: Too big allocation size %x (max allowed %x)\n", Size, Heap->MaximumAllocationSize);
        return NULL;
    }

    /* Lock the heap ourselves */
    if (!(Flags & HEAP_NO_SERIALIZE))
    {
        RtlEnterHeapLock(Heap->LockVariable, TRUE);
        HeapLocked = TRUE;

        /* Add no serialize flag so that the main routine won't try to acquire the lock again */
        Flags |= HEAP_NO_SERIALIZE;
    }

    /* Validate the heap if necessary */
    RtlpValidateHeap(Heap, FALSE);

    /* Get the existing heap entry */
    HeapEntry = (PHEAP_ENTRY)Ptr - 1;

    /* Validate it */
    if (RtlpValidateHeapEntry(Heap, HeapEntry))
    {
        /* Call main routine to do the stuff */
        Result = RtlReAllocateHeap(HeapPtr, Flags, Ptr, Size);

        if (Result)
        {
            /* Validate heap headers and then heap itself */
            RtlpValidateHeapHeaders(Heap, TRUE);
            RtlpValidateHeap(Heap, FALSE);
        }
    }

    /* Release the lock */
    if (HeapLocked) RtlLeaveHeapLock(Heap->LockVariable);

    return Result;
}

BOOLEAN NTAPI
RtlDebugFreeHeap(HANDLE HeapPtr,
                 ULONG Flags,
                 PVOID Ptr)
{
    PHEAP Heap = (PHEAP)HeapPtr;
    BOOLEAN HeapLocked = FALSE;
    PHEAP_ENTRY HeapEntry;
    BOOLEAN Result = FALSE;

    if (Heap->ForceFlags & HEAP_FLAG_PAGE_ALLOCS)
        return RtlpPageHeapFree(HeapPtr, Flags, Ptr);

    if (!RtlpCheckHeapSignature(Heap, "RtlFreeHeap"))
        return FALSE;

    /* Add skip validation flag */
    Flags |= Heap->ForceFlags | HEAP_SKIP_VALIDATION_CHECKS;

    /* Lock the heap ourselves */
    if (!(Flags & HEAP_NO_SERIALIZE))
    {
        RtlEnterHeapLock(Heap->LockVariable, TRUE);
        HeapLocked = TRUE;

        /* Add no serialize flag so that the main routine won't try to acquire the lock again */
        Flags |= HEAP_NO_SERIALIZE;
    }

    /* Validate the heap if necessary */
    RtlpValidateHeap(Heap, FALSE);

    /* Get the existing heap entry */
    HeapEntry = (PHEAP_ENTRY)Ptr - 1;

    /* Validate it */
    if (RtlpValidateHeapEntry(Heap, HeapEntry))
    {
        /* If it succeeded - call the main routine */
        Result = RtlFreeHeap(HeapPtr, Flags, Ptr);

        /* Validate heap headers and then heap itself */
        RtlpValidateHeapHeaders(Heap, TRUE);
        RtlpValidateHeap(Heap, FALSE);
    }

    /* Release the lock */
    if (HeapLocked) RtlLeaveHeapLock(Heap->LockVariable);

    return Result;
}

BOOLEAN NTAPI
RtlDebugGetUserInfoHeap(PVOID HeapHandle,
                        ULONG Flags,
                        PVOID BaseAddress,
                        PVOID *UserValue,
                        PULONG UserFlags)
{
    PHEAP Heap = (PHEAP)HeapHandle;
    BOOLEAN HeapLocked = FALSE;
    PHEAP_ENTRY HeapEntry;
    BOOLEAN Result = FALSE;

    if (Heap->ForceFlags & HEAP_FLAG_PAGE_ALLOCS)
        return RtlpPageHeapGetUserInfo(HeapHandle, Flags, BaseAddress, UserValue, UserFlags);

    if (!RtlpCheckHeapSignature(Heap, "RtlGetUserInfoHeap"))
        return FALSE;

    /* Add skip validation flag */
    Flags |= Heap->ForceFlags | HEAP_SKIP_VALIDATION_CHECKS;

    /* Lock the heap ourselves */
    if (!(Flags & HEAP_NO_SERIALIZE))
    {
        RtlEnterHeapLock(Heap->LockVariable, TRUE);
        HeapLocked = TRUE;

        /* Add no serialize flag so that the main routine won't try to acquire the lock again */
        Flags |= HEAP_NO_SERIALIZE;
    }

    /* Validate the heap if necessary */
    RtlpValidateHeap(Heap, FALSE);

    /* Get the existing heap entry */
    HeapEntry = (PHEAP_ENTRY)BaseAddress - 1;

    /* Validate it */
    if (RtlpValidateHeapEntry(Heap, HeapEntry))
    {
        /* If it succeeded - call the main routine */
        Result = RtlGetUserInfoHeap(HeapHandle, Flags, BaseAddress, UserValue, UserFlags);
    }

    /* Release the lock */
    if (HeapLocked) RtlLeaveHeapLock(Heap->LockVariable);

    return Result;
}

BOOLEAN NTAPI
RtlDebugSetUserValueHeap(PVOID HeapHandle,
                         ULONG Flags,
                         PVOID BaseAddress,
                         PVOID UserValue)
{
    PHEAP Heap = (PHEAP)HeapHandle;
    BOOLEAN HeapLocked = FALSE;
    PHEAP_ENTRY HeapEntry;
    BOOLEAN Result = FALSE;

    if (Heap->ForceFlags & HEAP_FLAG_PAGE_ALLOCS)
        return RtlpPageHeapSetUserValue(HeapHandle, Flags, BaseAddress, UserValue);

    if (!RtlpCheckHeapSignature(Heap, "RtlSetUserValueHeap"))
        return FALSE;

    /* Add skip validation flag */
    Flags |= Heap->ForceFlags | HEAP_SKIP_VALIDATION_CHECKS;

    /* Lock the heap ourselves */
    if (!(Flags & HEAP_NO_SERIALIZE))
    {
        RtlEnterHeapLock(Heap->LockVariable, TRUE);
        HeapLocked = TRUE;

        /* Add no serialize flag so that the main routine won't try to acquire the lock again */
        Flags |= HEAP_NO_SERIALIZE;
    }

    /* Validate the heap if necessary */
    RtlpValidateHeap(Heap, FALSE);

    /* Get the existing heap entry */
    HeapEntry = (PHEAP_ENTRY)BaseAddress - 1;

    /* Validate it */
    if (RtlpValidateHeapEntry(Heap, HeapEntry))
    {
        /* If it succeeded - call the main routine */
        Result = RtlSetUserValueHeap(HeapHandle, Flags, BaseAddress, UserValue);

        /* Validate the heap */
        RtlpValidateHeap(Heap, FALSE);
    }

    /* Release the lock */
    if (HeapLocked) RtlLeaveHeapLock(Heap->LockVariable);

    return Result;
}

BOOLEAN
NTAPI
RtlDebugSetUserFlagsHeap(PVOID HeapHandle,
                         ULONG Flags,
                         PVOID BaseAddress,
                         ULONG UserFlagsReset,
                         ULONG UserFlagsSet)
{
    PHEAP Heap = (PHEAP)HeapHandle;
    BOOLEAN HeapLocked = FALSE;
    PHEAP_ENTRY HeapEntry;
    BOOLEAN Result = FALSE;

    if (Heap->ForceFlags & HEAP_FLAG_PAGE_ALLOCS)
        return RtlpPageHeapSetUserFlags(HeapHandle, Flags, BaseAddress, UserFlagsReset, UserFlagsSet);

    /* Check if this heap allows flags to be set at all */
    if (UserFlagsSet & ~HEAP_SETTABLE_USER_FLAGS ||
        UserFlagsReset & ~HEAP_SETTABLE_USER_FLAGS)
    {
        return FALSE;
    }

    if (!RtlpCheckHeapSignature(Heap, "RtlSetUserFlagsHeap"))
        return FALSE;

    /* Add skip validation flag */
    Flags |= Heap->ForceFlags | HEAP_SKIP_VALIDATION_CHECKS;

    /* Lock the heap ourselves */
    if (!(Flags & HEAP_NO_SERIALIZE))
    {
        RtlEnterHeapLock(Heap->LockVariable, TRUE);
        HeapLocked = TRUE;

        /* Add no serialize flag so that the main routine won't try to acquire the lock again */
        Flags |= HEAP_NO_SERIALIZE;
    }

    /* Validate the heap if necessary */
    RtlpValidateHeap(Heap, FALSE);

    /* Get the existing heap entry */
    HeapEntry = (PHEAP_ENTRY)BaseAddress - 1;

    /* Validate it */
    if (RtlpValidateHeapEntry(Heap, HeapEntry))
    {
        /* If it succeeded - call the main routine */
        Result = RtlSetUserFlagsHeap(HeapHandle, Flags, BaseAddress, UserFlagsReset, UserFlagsSet);

        /* Validate the heap */
        RtlpValidateHeap(Heap, FALSE);
    }

    /* Release the lock */
    if (HeapLocked) RtlLeaveHeapLock(Heap->LockVariable);

    return Result;
}

SIZE_T NTAPI
RtlDebugSizeHeap(HANDLE HeapPtr,
                 ULONG Flags,
                 PVOID Ptr)
{
    PHEAP Heap = (PHEAP)HeapPtr;
    BOOLEAN HeapLocked = FALSE;
    PHEAP_ENTRY HeapEntry;
    SIZE_T Result = ~(SIZE_T)0;

    if (Heap->ForceFlags & HEAP_FLAG_PAGE_ALLOCS)
        return RtlpPageHeapSize(HeapPtr, Flags, Ptr);

    /* Check heap signature */
    if (!RtlpCheckHeapSignature(Heap, "RtlSizeHeap"))
        return FALSE;

    /* Add skip validation flag */
    Flags |= Heap->ForceFlags | HEAP_SKIP_VALIDATION_CHECKS;

    /* Lock the heap ourselves */
    if (!(Flags & HEAP_NO_SERIALIZE))
    {
        RtlEnterHeapLock(Heap->LockVariable, TRUE);
        HeapLocked = TRUE;

        /* Add no serialize flag so that the main routine won't try to acquire the lock again */
        Flags |= HEAP_NO_SERIALIZE;
    }

    /* Validate the heap if necessary */
    RtlpValidateHeap(Heap, FALSE);

    /* Get the existing heap entry */
    HeapEntry = (PHEAP_ENTRY)Ptr - 1;

    /* Validate it */
    if (RtlpValidateHeapEntry(Heap, HeapEntry))
    {
        /* If it succeeded - call the main routine */
        Result = RtlSizeHeap(HeapPtr, Flags, Ptr);
    }

    /* Release the lock */
    if (HeapLocked) RtlLeaveHeapLock(Heap->LockVariable);

    return Result;
}

BOOLEAN
NTAPI
RtlpCheckHeapSignature(IN HANDLE HeapHandle, IN PCSTR Caller OPTIONAL)
{
    const PHEAP Heap = (PHEAP)HeapHandle;

    ASSERT(Heap);

    if (Heap->Signature == HEAP_SIGNATURE)
        return TRUE;

    DbgPrint("HEAP: Invalid heap [%p] signature 0x%lX", Heap, Heap->Signature);

    if (Caller != NULL)
        DbgPrint(", passed to %s", Caller);

    DbgPrint("\n");

    RtlpBreakPointHeap(&Heap->Signature);

    return FALSE;
}

void
NTAPI
RtlpBreakPointHeap(IN PVOID BadAddress OPTIONAL)
{
    if (!NtCurrentPeb()->BeingDebugged)
        return;

    RtlpHeapInvalidBreakPoint = TRUE;

    RtlpHeapInvalidBadAddress = BadAddress;

    DbgBreakPoint();

    RtlpHeapInvalidBreakPoint = FALSE;
}

/* EOF */
