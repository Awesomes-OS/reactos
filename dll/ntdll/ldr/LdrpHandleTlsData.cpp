#include <ldrp.h>
#include <ndk/psfuncs.h>

NTSTATUS
NTAPI
LdrpHandleTlsData(IN OUT PLDR_DATA_TABLE_ENTRY ModuleEntry)
{
    PIMAGE_TLS_DIRECTORY TlsDirectory;
    ULONG DirectorySize;
    PPROCESS_TLS_INFORMATION TlsInfo;
    PTLS_ENTRY PendingReleaseTlsEntry = NULL;
    BOOLEAN AllocatedBitmap = FALSE, AllocatedTlsEntry = FALSE;
    NTSTATUS Status;

    if (LdrpActiveThreadCount == 0)
        return STATUS_SUCCESS;

    // Discover the TLS directory address for this module.

    Status = RtlImageDirectoryEntryToDataEx(
        ModuleEntry->DllBase,
        TRUE,
        IMAGE_DIRECTORY_ENTRY_TLS,
        &DirectorySize,
        (PVOID*)&TlsDirectory
    );

    // If we've got no TLS directory then we're done.

    if (!TlsDirectory || !DirectorySize || !NT_SUCCESS(Status))
        return STATUS_SUCCESS;

    RtlAcquireSRWLockExclusive(&LdrpTlsLock);

    // We'll be using the process heap.

    // We've got an optimization for one active thread, which is the case for
    // traditional static-link DLLs that use __declspec(thread).

    // Allocate memory for our thread data block.

    const LONG TlsInfoSize = FIELD_OFFSET(PROCESS_TLS_INFORMATION, ThreadData[LdrpActiveThreadCount]);
    TlsInfo = (PPROCESS_TLS_INFORMATION)RtlAllocateHeap(
        RtlGetProcessHeap(),
        0,
        TlsInfoSize
    );

    if (!TlsInfo)
    {
        RtlReleaseSRWLockExclusive(&LdrpTlsLock);
        return STATUS_NO_MEMORY;
    }

    do
    {
        ULONG TlsIndex;
        SIZE_T TlsRawDataLength;
        PTLS_ENTRY TlsEntry;
        ULONG ThreadIndex;
        ULONG ThreadsCleanedUp = 0;

        // Allocate a TLS index (or a new TLS bitmap).

        ULONG TlsBitmapLength = LdrpTlsBitmap.SizeOfBitMap;

        Status = LdrpAllocateTlsEntry(
            TlsDirectory,
            ModuleEntry,
            &TlsIndex,
            &AllocatedBitmap,
            &TlsEntry
        );

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("TLS entry allocation failed [%#X], aborting...\n", Status);
            break;
        }

        AllocatedTlsEntry = TRUE;

        TlsInfo->ThreadDataCount = LdrpActiveThreadCount;

        if (AllocatedBitmap)
        {
            TlsInfo->OperationType = ProcessTlsReplaceVector;
            TlsInfo->TlsVectorLength = TlsBitmapLength;

            TlsBitmapLength = LdrpTlsBitmap.SizeOfBitMap;
        }
        else
        {
            TlsInfo->OperationType = ProcessTlsReplaceIndex;
            TlsInfo->TlsIndex = TlsIndex;
        }

        Status = STATUS_SUCCESS;

        // Calculate the size of the raw TLS data for this module.

        TlsRawDataLength = TlsEntry->TlsDirectory.EndAddressOfRawData -
            TlsEntry->TlsDirectory.StartAddressOfRawData;

        // Prepare data for each running thread.

        for (ThreadIndex = 0; ThreadIndex < TlsInfo->ThreadDataCount; ThreadIndex++)
        {
            const PVOID TlsMemoryBlock = RtlAllocateHeap(
                RtlGetProcessHeap(),
                0,
                TlsRawDataLength
            );

            if (!TlsMemoryBlock)
            {
                Status = STATUS_NO_MEMORY;
                break;
            }

            // Copy the initializer raw data into the newly allocated TLS memory block for thread #ThreadIndex.

            __try
            {
                RtlCopyMemory(
                    TlsMemoryBlock,
                    (PVOID)TlsEntry->TlsDirectory.StartAddressOfRawData,
                    TlsRawDataLength
                );
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                Status = GetExceptionCode();
            }
            _SEH2_END

            if (!NT_SUCCESS(Status))
            {
                DPRINT1("RtlCopyMemory failed [%#X], exiting...\n", Status);

                RtlFreeHeap(RtlGetProcessHeap(), 0, TlsMemoryBlock);

                break;
            }

            if (AllocatedBitmap)
            {
                PVOID* TlsVector = (PVOID*)LdrpGetNewTlsVector(TlsBitmapLength);

                if (!TlsVector)
                {
                    RtlFreeHeap(RtlGetProcessHeap(), 0, TlsMemoryBlock);

                    if (NT_SUCCESS(Status))
                        Status = STATUS_NO_MEMORY;

                    break;
                }

                TlsVector[TlsIndex] = TlsMemoryBlock;

                TlsInfo->ThreadData[ThreadIndex].TlsVector = TlsVector;
            }
            else
            {
                TlsInfo->ThreadData[ThreadIndex].TlsModulePointer = TlsMemoryBlock;
            }

            TlsInfo->ThreadData[ThreadIndex].Flags = 0;
        }

        if (!NT_SUCCESS(Status))
            break;

        TlsInfo->Reserved = 0;

        ASSERT(ThreadIndex == TlsInfo->ThreadDataCount);
        ASSERT(TlsInfoSize == FIELD_OFFSET(PROCESS_TLS_INFORMATION, ThreadData[TlsInfo->ThreadDataCount]));

        // Perform the actual work of swapping the thread TLS data.

        Status = NtSetInformationProcess(
            NtCurrentProcess(),
            ProcessTlsInformation,
            TlsInfo,
            TlsInfoSize
        );

        // Let's handle each thread that we replaced, as the
        // ProcessTlsInformation call fills our buffer with the old data
        // after performing a swap.

        while (ThreadIndex-- > 0)
        {
            const PTHREAD_TLS_INFORMATION ResultTlsInformation = &TlsInfo->ThreadData[ThreadIndex];

            if (ResultTlsInformation->Flags & 0x1)
            {
                // Failure in kernel mode, potential TLS memory leak
                DPRINT1("ResultTlsInformation->Flags [%#x] has failure bit set, status %#x.\n",
                    ResultTlsInformation->Flags, Status);
                continue;
            }

            // Same as ThreadTlsData->TlsModulePointer (union)
            if (!ResultTlsInformation->TlsVector)
                continue;

            // Success, schedule TLS block deletion on thread exit
            if ((ResultTlsInformation->Flags & 0x2) && AllocatedBitmap)
            {
                LdrpQueueDeferredTlsData(
                    ResultTlsInformation->TlsVector,
                    ResultTlsInformation->ThreadId
                );

                continue;
            }

            if (!ResultTlsInformation->Flags)
            {
                // Thread disposed (not iterated through using PsGetNextProcessThread
                ThreadsCleanedUp++;

                if (AllocatedBitmap)
                {
                    // Free the old TLS memory block, pointed to by the item in TLS_VECTOR
                    RtlFreeHeap(
                        RtlGetProcessHeap(),
                        0,
                        ResultTlsInformation->TlsVector[TlsIndex]
                    );

                    // Free the whole TLS_VECTOR allocated with LdrpGetNewTlsVector above
                    RtlFreeHeap(
                        RtlGetProcessHeap(),
                        0,
                        CONTAINING_RECORD(ResultTlsInformation->TlsVector, TLS_VECTOR, ModuleTlsData)
                    );
                }
            }

            if (!AllocatedBitmap)
            {
                // Free the old TLS memory block, pointed to by the TlsModulePointer
                RtlFreeHeap(
                    RtlGetProcessHeap(),
                    0,
                    ResultTlsInformation->TlsModulePointer
                );
            }
        }

        if (NT_SUCCESS(Status) && (ThreadsCleanedUp > 0))
        {
            DPRINT1("TLS Thread Post-Shutdown: success [%#X], decrementing LdrpActiveThreadCount: (%u)-(%u).\n",
                Status, LdrpActiveThreadCount, ThreadsCleanedUp);

            InterlockedExchangeAdd((PLONG)&LdrpActiveThreadCount, -((LONG)ThreadsCleanedUp));
        }
    }
    while (FALSE);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("TLS memory block swap failure [%#X], rolling back...\n", Status);

        if (AllocatedTlsEntry)
        {
            if (!NT_SUCCESS(LdrpReleaseTlsEntry(ModuleEntry, &PendingReleaseTlsEntry)))
                DPRINT1("TLS rollback memory release failure [%#X].\n");
        }

        if (AllocatedBitmap)
        {
            LdrpTlsBitmap.SizeOfBitMap -= TLS_BITMAP_GROW_INCREMENT;
        }
    }
    else
    {
        ModuleEntry->TlsIndex = -1; // just nonzero would be sufficient, but let's mimic the original
        Status = STATUS_SUCCESS;
    }

    RtlReleaseSRWLockExclusive(&LdrpTlsLock);

    RtlFreeHeap(RtlGetProcessHeap(), 0, TlsInfo);

    if (PendingReleaseTlsEntry)
        RtlFreeHeap(RtlGetProcessHeap(), 0, PendingReleaseTlsEntry);

    return Status;
}
