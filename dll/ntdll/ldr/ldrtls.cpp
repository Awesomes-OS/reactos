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

typedef struct _TLS_RECLAIM_TABLE_ENTRY
{
    PTLS_VECTOR TlsVector;
    RTL_SRWLOCK Lock;
} TLS_RECLAIM_TABLE_ENTRY, *PTLS_RECLAIM_TABLE_ENTRY;

/* GLOBALS *******************************************************************/

LIST_ENTRY LdrpTlsList;

RTL_BITMAP LdrpTlsBitmap;
ULONG LdrpActiveThreadCount = 0;
ULONG LdrpActualBitmapSize = 0;
TLS_RECLAIM_TABLE_ENTRY LdrpDelayedTlsReclaimTable[16];

RTL_SRWLOCK LdrpTlsLock = RTL_SRWLOCK_INIT;

/* FUNCTIONS *****************************************************************/

PVOID* LdrpGetNewTlsVector(IN ULONG TlsBitmapLength)
{
    auto* TlsVector = (PTLS_VECTOR)RtlAllocateHeap(
        RtlGetProcessHeap(),
        HEAP_ZERO_MEMORY,
        FIELD_OFFSET(TLS_VECTOR, ModuleTlsData[TlsBitmapLength])
    );

    if (!TlsVector)
        return NULL;

    TlsVector->Length = TlsBitmapLength;
    return TlsVector->ModuleTlsData;
}

NTSTATUS
LdrpGrowTlsBitmap(IN ULONG newLength)
{
    const ULONG AlignedLength = LDRP_BITMAP_CALC_ALIGN(newLength, LDRP_BITMAP_BITALIGN);

    const PULONG NewBitmapBuffer = (PULONG)RtlAllocateHeap(
        RtlGetProcessHeap(),
        HEAP_ZERO_MEMORY, // we will clear bits later
        AlignedLength * sizeof(ULONG)
    );

    if (!NewBitmapBuffer)
    {
        return STATUS_NO_MEMORY;
    }

    LdrpActualBitmapSize = AlignedLength;

    if (LdrpTlsBitmap.SizeOfBitMap > 0)
    {
        // Copy the contents of the previous buffer into the new one.
        RtlCopyMemory(
            NewBitmapBuffer,
            LdrpTlsBitmap.Buffer,
            LDRP_BITMAP_CALC_ALIGN(LdrpTlsBitmap.SizeOfBitMap, 8)
        );

        // Free the old buffer.
        RtlFreeHeap(RtlGetProcessHeap(), 0, LdrpTlsBitmap.Buffer);
    }

    // Reinitialize the bitmap as we've changed the buffer pointer.

    RtlInitializeBitMap(
        &LdrpTlsBitmap,
        NewBitmapBuffer,
        newLength
    );

    return STATUS_SUCCESS;
}

NTSTATUS
LdrpAcquireTlsIndex(
    OUT PULONG TlsIndex,
    OUT PBOOLEAN AllocatedBitmap
)
{
    ULONG Index;

    if (LdrpTlsBitmap.SizeOfBitMap > 0)
    {
        Index = RtlFindClearBitsAndSet(&LdrpTlsBitmap, 1, 0);

        // If we found space in the existing bitmap then there is no reason to
        // expand buffers, so we'll just return with the existing data.

        if (Index != 0xFFFFFFFF)
        {
            *TlsIndex = Index;
            *AllocatedBitmap = FALSE;

            return STATUS_SUCCESS;
        }
    }

    Index = LdrpTlsBitmap.SizeOfBitMap;

    const ULONG NewLength = LdrpTlsBitmap.SizeOfBitMap + TLS_BITMAP_GROW_INCREMENT;

    // Check if we need to grow the bitmap itself or if the bitmap still
    // has space
    if (LDRP_BITMAP_CALC_ALIGN(NewLength, LDRP_BITMAP_BITALIGN) > LdrpActualBitmapSize)
    {
        // We'll need to grow it.  Let's go do so now.

        NTSTATUS Status;
        if (!NT_SUCCESS(Status = LdrpGrowTlsBitmap(NewLength)))
            return Status;
    }
    else
    {
        LdrpTlsBitmap.SizeOfBitMap += TLS_BITMAP_GROW_INCREMENT;
    }

    RtlSetBit(&LdrpTlsBitmap, Index);

    *TlsIndex = Index;
    *AllocatedBitmap = TRUE;

    return STATUS_SUCCESS;
}

VOID
LdrpReleaseTlsIndex(IN ULONG TlsIndex)
{
    RtlClearBit(&LdrpTlsBitmap, TlsIndex);
}

NTSTATUS
LdrpAllocateTlsEntry(
    IN PIMAGE_TLS_DIRECTORY TlsDirectory,
    IN PLDR_DATA_TABLE_ENTRY ModuleEntry,
    OUT PULONG TlsIndex,
    OUT PBOOLEAN AllocatedBitmap OPTIONAL,
    OUT PTLS_ENTRY* TlsEntry
)
{
    PTLS_ENTRY Entry = NULL;
    NTSTATUS Status;

    ASSERT(TlsDirectory);

    __try
    {
        Entry = (PTLS_ENTRY)RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(TLS_ENTRY));

        if (!Entry)
            _SEH2_YIELD(return STATUS_NO_MEMORY);

        Status = STATUS_SUCCESS;

        RtlCopyMemory(&Entry->TlsDirectory, TlsDirectory, sizeof(IMAGE_TLS_DIRECTORY));
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = GetExceptionCode();
    }
    _SEH2_END

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("TLS entry creation failed [0x%08lX], exiting...\n", Status);

        RtlFreeHeap(RtlGetProcessHeap(), 0, Entry);

        return Status;
    }

    // Validate that the TLS directory entry is sane.

    if (Entry->TlsDirectory.StartAddressOfRawData > Entry->TlsDirectory.EndAddressOfRawData)
    {
        if (ModuleEntry)
        {
            DPRINT1("TLS Directory of %ls has erroneous data range: [%lu, %lu]\n",
                    ModuleEntry->FullDllName.Buffer, TlsDirectory->StartAddressOfRawData,
                    TlsDirectory->EndAddressOfRawData);
        }
        else
        {
            DPRINT1("TLS Directory [UNKNOWN MODULE] has erroneous data range: [%lu, %lu]\n",
                    TlsDirectory->StartAddressOfRawData, TlsDirectory->EndAddressOfRawData);
        }

        RtlFreeHeap(RtlGetProcessHeap(), 0, Entry);

        return STATUS_INVALID_IMAGE_FORMAT;
    }

    Entry->ModuleEntry = ModuleEntry;

    // Insert the entry into our list.

    InsertTailList(&LdrpTlsList, &Entry->TlsEntryLinks);

    if (AllocatedBitmap)
    {
        Status = LdrpAcquireTlsIndex(TlsIndex, AllocatedBitmap);

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("TLS index acquisition failed during entry creation [0x%08lX].\n", Status);

            RemoveEntryList(&Entry->TlsEntryLinks);

            RtlFreeHeap(RtlGetProcessHeap(), 0, Entry);

            return Status;
        }
    }
    else
    {
        *TlsIndex += 1;
    }

    // We reuse the 'Characteristics' field for the real TLS index.

    Entry->TlsDirectory.Characteristics = *TlsIndex;

    __try
    {
        *(PULONG)Entry->TlsDirectory.AddressOfIndex = *TlsIndex;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = GetExceptionCode();
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("TLS directory index persistence failed during entry creation [0x%08lX].\n", Status);

        if (AllocatedBitmap)
        {
            LdrpReleaseTlsIndex(*TlsIndex);

            if (*AllocatedBitmap)
                LdrpTlsBitmap.SizeOfBitMap -= TLS_BITMAP_GROW_INCREMENT;
        }

        RemoveEntryList(&Entry->TlsEntryLinks);

        RtlFreeHeap(RtlGetProcessHeap(), 0, Entry);

        return Status;
    }

    if (TlsEntry)
        *TlsEntry = Entry;

    return STATUS_SUCCESS;
}

PTLS_ENTRY
FASTCALL
LdrpFindTlsEntry(IN PLDR_DATA_TABLE_ENTRY ModuleEntry)
{
    PTLS_ENTRY TlsEntry;

    PLIST_ENTRY ListHead = &LdrpTlsList;

    for (TlsEntry = CONTAINING_RECORD(LdrpTlsList.Flink, TLS_ENTRY, TlsEntryLinks);
         &TlsEntry->TlsEntryLinks != ListHead;
         TlsEntry = CONTAINING_RECORD(TlsEntry->TlsEntryLinks.Flink, TLS_ENTRY, TlsEntryLinks))
    {
        if (TlsEntry->ModuleEntry == ModuleEntry)
            return TlsEntry;
    }

    return 0;
}

NTSTATUS
NTAPI
LdrpReleaseTlsEntry(IN PLDR_DATA_TABLE_ENTRY ModuleEntry, OUT PTLS_ENTRY* FoundTlsEntry OPTIONAL)
{
    if (!FoundTlsEntry)
        RtlAcquireSRWLockExclusive(&LdrpTlsLock);

    // Find the corresponding TLS_ENTRY for this module entry.

    PTLS_ENTRY TlsEntry = LdrpFindTlsEntry(ModuleEntry);

    if (!TlsEntry)
    {
        if (!FoundTlsEntry)
            RtlReleaseSRWLockExclusive(&LdrpTlsLock);
        return STATUS_NOT_FOUND;
    }

    // Remove it from the global list of outstanding TLS entries.

    RemoveEntryList(&TlsEntry->TlsEntryLinks);

    // Deallocate the TLS index.

    LdrpReleaseTlsIndex(TlsEntry->TlsDirectory.Characteristics);

    if (!FoundTlsEntry)
        RtlReleaseSRWLockExclusive(&LdrpTlsLock);

    // Deallocate the TLS_ENTRY object itself, unless FoundTlsEntry is non-NULL
    // Call from HandleTlsData does that, because of SRW lock already being held.
    if (FoundTlsEntry)
        *FoundTlsEntry = TlsEntry;
    else
        RtlFreeHeap(RtlGetProcessHeap(), 0, TlsEntry);

    // We're done.

    return STATUS_SUCCESS;
}

VOID
LdrpQueueDeferredTlsData(
    IN OUT PVOID TlsVector,
    IN OUT PVOID ThreadId
)
{
    const PTLS_RECLAIM_TABLE_ENTRY ReclaimEntry =
        &LdrpDelayedTlsReclaimTable[((ULONG_PTR)(ThreadId) >> 2) & 0xF];

    const PTLS_VECTOR RealTlsVector = CONTAINING_RECORD(
        TlsVector,
        TLS_VECTOR,
        ModuleTlsData
    );

    RealTlsVector->ThreadId = ThreadId;

#if 1
    RtlAcquireSRWLockExclusive(&ReclaimEntry->Lock);
#endif

    RealTlsVector->PreviousDeferredTlsVector = ReclaimEntry->TlsVector;
    ReclaimEntry->TlsVector = RealTlsVector;

#if 1
    RtlReleaseSRWLockExclusive(&ReclaimEntry->Lock);
#endif
}

BOOL
LdrpCleanupThreadTlsData(VOID)
{
    BOOL Result = TRUE;
    PTLS_VECTOR TargetReclaimTlsVector = NULL;
    PTLS_VECTOR PreviousReclaimVector = NULL;
    const HANDLE CurrentThreadHandle = NtCurrentTeb()->RealClientId.UniqueThread;
    const PTLS_RECLAIM_TABLE_ENTRY FoundReclaimEntry =
        &LdrpDelayedTlsReclaimTable[((ULONG_PTR)CurrentThreadHandle >> 2) & 0xF];
    PTLS_VECTOR CurrentReclaimVector = FoundReclaimEntry->TlsVector;

#if 1
    RtlAcquireSRWLockExclusive(&FoundReclaimEntry->Lock);
#endif

    while (CurrentReclaimVector)
    {
        const PTLS_VECTOR NextReclaimVector = CurrentReclaimVector->PreviousDeferredTlsVector;

        if (CurrentReclaimVector->ThreadId == CurrentThreadHandle)
        {
            if (PreviousReclaimVector)
                PreviousReclaimVector->PreviousDeferredTlsVector = NextReclaimVector;
            else
                FoundReclaimEntry->TlsVector = NextReclaimVector;

            CurrentReclaimVector->PreviousDeferredTlsVector = TargetReclaimTlsVector;
            TargetReclaimTlsVector = CurrentReclaimVector;
            CurrentReclaimVector = PreviousReclaimVector;
        }

        PreviousReclaimVector = CurrentReclaimVector;
        CurrentReclaimVector = NextReclaimVector;
    }

#if 1
    RtlReleaseSRWLockExclusive(&FoundReclaimEntry->Lock);
#endif

    while (TargetReclaimTlsVector)
    {
        const PTLS_VECTOR NextReclaimTlsVector = TargetReclaimTlsVector->PreviousDeferredTlsVector;

        Result = RtlFreeHeap(RtlGetProcessHeap(), 0, TargetReclaimTlsVector);
        TargetReclaimTlsVector = NextReclaimTlsVector;
    }

    return Result;
}

NTSTATUS
NTAPI
LdrpInitializeTls(VOID)
{
    const PLIST_ENTRY ListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
    ULONG TlsIndex = 0;
    NTSTATUS Status;

    /* Initialize the TLS List */
    InitializeListHead(&LdrpTlsList);

    /* Loop all the modules */
    for (PLIST_ENTRY NextEntry = ListHead->Flink; ListHead != NextEntry; NextEntry = NextEntry->Flink)
    {
        ULONG Size;

        /* Get the entry */
        PLDR_DATA_TABLE_ENTRY Module = CONTAINING_RECORD(NextEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

        /* Get the TLS directory */
        auto* TlsDirectory = (PIMAGE_TLS_DIRECTORY)RtlImageDirectoryEntryToData(Module->DllBase,
                                                                               TRUE,
                                                                               IMAGE_DIRECTORY_ENTRY_TLS,
                                                                               &Size);

        /* Check if we have a directory */
        if (!TlsDirectory || !Size) continue;

        /* Show debug message */
        if (LdrpDebugFlags.LogTrace)
        {
            DPRINT1("LDR: Tls Found in %wZ at " LDR_HEXPTR_FMT "\n",
                    &Module->BaseDllName,
                    TlsDirectory);
        }

        /* Allocate an entry */
        Status = LdrpAllocateTlsEntry(TlsDirectory, Module, &TlsIndex, NULL, NULL);
        if (!NT_SUCCESS(Status)) return Status;

        /* Lock the DLL and mark it for TLS Usage */
        Module->DdagNode->LoadCount = LDR_LOADCOUNT_MAX;
        Module->TlsIndex = -1;
    }

    if (!TlsIndex)
    {
        RtlInitializeBitMap(&LdrpTlsBitmap, NULL, 0);
    }
    else
    {
        // First-time equivalent of LdrpAcquireTlsIndex

        const ULONG BitMapSize = TlsIndex + TLS_BITMAP_GROW_INCREMENT;

        if (!NT_SUCCESS(Status = LdrpGrowTlsBitmap(BitMapSize)))
            return Status;

        RtlSetBits(&LdrpTlsBitmap, 0, TlsIndex);
    }

    /* Done setting up TLS, allocate entries */
    return LdrpAllocateTls();
}

NTSTATUS
NTAPI
LdrpAllocateTls(VOID)
{
    const PLIST_ENTRY ListHead = &LdrpTlsList;
    PLIST_ENTRY NextEntry;
    PVOID* TlsVector;

    RtlAcquireSRWLockShared(&LdrpTlsLock);

    /* Check if we have any entries */
    if (!LdrpTlsBitmap.SizeOfBitMap)
        goto success_exit;

    /* Allocate the vector array */
    TlsVector = LdrpGetNewTlsVector(LdrpTlsBitmap.SizeOfBitMap);
    if (!TlsVector)
    {
        RtlReleaseSRWLockShared(&LdrpTlsLock);
        return STATUS_NO_MEMORY;
    }

    /* Loop the TLS Array */
    for (NextEntry = ListHead->Flink; NextEntry != ListHead; NextEntry = NextEntry->Flink)
    {
        PTLS_ENTRY TlsData;
        SIZE_T TlsDataSize;

        /* Get the entry */
        TlsData = CONTAINING_RECORD(NextEntry, TLS_ENTRY, TlsEntryLinks);

        /* Allocate this vector */
        TlsDataSize = TlsData->TlsDirectory.EndAddressOfRawData -
            TlsData->TlsDirectory.StartAddressOfRawData;
        TlsVector[TlsData->TlsDirectory.Characteristics] = RtlAllocateHeap(RtlGetProcessHeap(),
                                                                           0,
                                                                           TlsDataSize);
        if (!TlsVector[TlsData->TlsDirectory.Characteristics])
        {
            /* Out of memory */

            ULONG i = 0;
            for (; i < LdrpTlsBitmap.SizeOfBitMap; ++i)
                if (TlsVector[i])
                    RtlFreeHeap(RtlGetProcessHeap(), 0, TlsVector[i]);

            RtlReleaseSRWLockShared(&LdrpTlsLock);
            return STATUS_NO_MEMORY;
        }

        /* Show debug message */
        if (LdrpDebugFlags.LogTrace)
        {
            DPRINT1("LDR: TlsVector " LDR_HEXPTR_FMT " Index %lu = " LDR_HEXPTR_FMT " copied from %x to " LDR_HEXPTR_FMT "\n",
                    TlsVector,
                    TlsData->TlsDirectory.Characteristics,
                    &TlsVector[TlsData->TlsDirectory.Characteristics],
                    TlsData->TlsDirectory.StartAddressOfRawData,
                    TlsVector[TlsData->TlsDirectory.Characteristics]);
        }

        /* Copy the data */
        RtlCopyMemory(TlsVector[TlsData->TlsDirectory.Characteristics],
                      (PVOID)TlsData->TlsDirectory.StartAddressOfRawData,
                      TlsDataSize);
    }

    NtCurrentTeb()->ThreadLocalStoragePointer = TlsVector;

success_exit:
    InterlockedIncrement((PLONG)&LdrpActiveThreadCount);
    RtlReleaseSRWLockShared(&LdrpTlsLock);
    /* Done */
    return STATUS_SUCCESS;
}

VOID
NTAPI
LdrpFreeTls(VOID)
{
    PTEB Teb = NtCurrentTeb();

    RtlAcquireSRWLockShared(&LdrpTlsLock);

    /* Get a pointer to the vector array */
    PVOID* TlsVector = (PVOID*)Teb->ThreadLocalStoragePointer;

    InterlockedDecrement((PLONG)&LdrpActiveThreadCount);
    Teb->ThreadLocalStoragePointer = NULL;

    RtlReleaseSRWLockShared(&LdrpTlsLock);

    if (TlsVector)
    {
        /* Loop through it */
        for (ULONG i = 0; i < LdrpTlsBitmap.SizeOfBitMap; ++i)
            if (TlsVector[i])
                RtlFreeHeap(RtlGetProcessHeap(), 0, TlsVector[i]);

        auto* RealTlsVector = CONTAINING_RECORD(TlsVector, TLS_VECTOR, ModuleTlsData);

        /* Free the array itself */
        RtlFreeHeap(RtlGetProcessHeap(), 0, RealTlsVector);
    }

    LdrpCleanupThreadTlsData();
}


VOID
NTAPI
LdrpCallTlsInitializers(IN PLDR_DATA_TABLE_ENTRY Module,
                        IN ULONG Reason)
{
    RtlAcquireSRWLockShared(&LdrpTlsLock);

    /* Get the TLS Directory */
    const PTLS_ENTRY TlsEntry = LdrpFindTlsEntry(Module);

    RtlReleaseSRWLockShared(&LdrpTlsLock);

    /* Protect against invalid pointers */
    __try
    {
        /* Make sure it's valid */
        if (TlsEntry)
        {
            /* Get the array */
            auto* Array = (PIMAGE_TLS_CALLBACK *)TlsEntry->TlsDirectory.AddressOfCallBacks;
            if (Array)
            {
                /* Display debug */
                if (LdrpDebugFlags.LogTrace)
                {
                    DPRINT1("LDR: Tls Callbacks Found. Imagebase " LDR_HEXPTR_FMT " Tls " LDR_HEXPTR_FMT " CallBacks " LDR_HEXPTR_FMT "\n",
                            Module->DllBase, TlsEntry->TlsDirectory, Array);
                }

                /* Loop the array */
                while (*Array)
                {
                    /* Get the TLS Entrypoint */
                    const PIMAGE_TLS_CALLBACK Callback = *Array++;

                    /* Display debug */
                    if (LdrpDebugFlags.LogTrace)
                    {
                        DPRINT1("LDR: Calling Tls Callback Imagebase " LDR_HEXPTR_FMT " Function " LDR_HEXPTR_FMT "\n",
                                Module->DllBase, Callback);
                    }

                    /* Call it */
                    LdrpCallInitRoutine((PDLL_INIT_ROUTINE)Callback,
                                        Module->DllBase,
                                        Reason,
                                        NULL);
                }
            }
        }
    }
    __except(LdrpDebugExceptionFilter(GetExceptionCode(), GetExceptionInformation()))
    {
        DPRINT1("LDR: Exception 0x%x during Tls Callback(%u) for %wZ\n",
                GetExceptionCode(), Reason, &Module->BaseDllName);
    }
}


/* EOF */
