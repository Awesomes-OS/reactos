/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/ke/balmgr.c
* PURPOSE:         Balance Set Manager
* PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
*/

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

#define THREAD_BOOST_PRIORITY (LOW_REALTIME_PRIORITY - 1)
ULONG KiReadyScanLast;

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
KiScanReadyQueues(IN PKDPC Dpc,
                  IN PVOID DeferredContext,
                  IN PVOID SystemArgument1,
                  IN PVOID SystemArgument2)
{
    PULONG ScanLast = DeferredContext;
    ULONG ScanIndex = *ScanLast;
    ULONG Count = 10, Number = 16;
    PKPRCB Prcb = KiProcessorBlock[ScanIndex];
    ULONG Index = Prcb->QueueIndex;
    ULONG WaitLimit = KeTickCount.LowPart - 300;
    ULONG Summary;
    KIRQL OldIrql;
    PLIST_ENTRY ListHead, NextEntry;
    PSINGLE_LIST_ENTRY DeferredListEntry = NULL;

    /* Lock the dispatcher and PRCB */
    OldIrql = KiAcquireDispatcherLock();
    KiAcquirePrcbLock(Prcb);
    /* Check if there's any thread that need help */
    Summary = Prcb->ReadySummary & ((1 << THREAD_BOOST_PRIORITY) - 2);
    if (Summary)
    {
        /* Start scan loop */
        do
        {
            /* Normalize the index */
            if (Index > (THREAD_BOOST_PRIORITY - 1)) Index = 1;

            /* Loop for ready threads */
            if (Summary & PRIORITY_MASK(Index))
            {
                /* Sanity check */
                ASSERT(!IsListEmpty(&Prcb->DispatcherReadyListHead[Index]));

                /* Update summary and select list */
                Summary ^= PRIORITY_MASK(Index);
                ListHead = &Prcb->DispatcherReadyListHead[Index];
                NextEntry = ListHead->Flink;
                do
                {
                    PKTHREAD Thread;

                    /* Select a thread */
                    Thread = CONTAINING_RECORD(NextEntry,
                                               KTHREAD,
                                               WaitListEntry);
                    ASSERT(Thread->Priority == Index);

                    /* Check if the thread has been waiting too long */
                    if (WaitLimit >= Thread->WaitTime)
                    {
                        /* Remove the thread from the queue */
                        NextEntry = NextEntry->Blink;
                        ASSERT((Prcb->ReadySummary & PRIORITY_MASK(Index)));
                        if (RemoveEntryList(NextEntry->Flink))
                        {
                            /* The list is empty now */
                            Prcb->ReadySummary ^= PRIORITY_MASK(Index);
                        }

                        KiInsertDeferredReadyList(Thread);
                        Count--;
                    }

                    /* Go to the next entry */
                    NextEntry = NextEntry->Flink;
                    Number--;
                } while((NextEntry != ListHead) && (Number) && (Count));
            }

            /* Increase index */
            Index++;
        } while ((Summary) && (Number) && (Count));
    }

    ASSERT_IRQL_GREATER_OR_EQUAL(DISPATCH_LEVEL);

    /* Get the first entry and clear the list */
    DeferredListEntry = Prcb->DeferredReadyListHead.Next;
    Prcb->DeferredReadyListHead.Next = NULL;

    /* Release the locks and dispatcher */
    KiReleasePrcbLock(Prcb);
    KiReleaseDispatcherLock(OldIrql);

    while (DeferredListEntry)
    {
        // Similar to KiProcessDeferredReadyList with additional logic to initialize threads

        PKTHREAD Thread;
        THREAD_PRIORITY_VALUES Value;

        /* Get the thread and advance to the next entry */
        Thread = CONTAINING_RECORD(DeferredListEntry, KTHREAD, SwapListEntry);
        DeferredListEntry = DeferredListEntry->Next;

        ASSERT_THREAD(Thread);
        ASSERT(Thread->State == Ready || Thread->State == DeferredReady);
        ASSERT_IRQL_EQUAL(DISPATCH_LEVEL);

        const ULONGLONG CycleTime = KiCaptureThreadCycleTime(Thread).QuadPart;

        /* Lock the thread */
        KiAcquireThreadLock(Thread);

        ASSERT_IRQL_EQUAL(DISPATCH_LEVEL);

        /* Verify priority decrement and set the new one */
        Value = KiExtractThreadPriorityValues(Thread->PriorityDecrement);
        ASSERT(Value.Major + Value.Minor <= Thread->Priority);
        ASSERT(Thread->PriorityDecrement >= 0);
        ASSERT(Thread->PriorityDecrement <= Thread->Priority);

        Thread->PriorityDecrement += THREAD_BOOST_PRIORITY - Thread->Priority;

        Value = KiExtractThreadPriorityValues(Thread->PriorityDecrement);
        ASSERT(Thread->PriorityDecrement >= 0);
        ASSERT(Thread->PriorityDecrement <= THREAD_BOOST_PRIORITY);
        ASSERT(Value.Major + Value.Minor <= THREAD_BOOST_PRIORITY);

        // Update priority and quantum
        Thread->Priority = THREAD_BOOST_PRIORITY;

        const ULONGLONG QuantumTarget = Thread->QuantumTarget;
        const ULONGLONG NewQuantumTarget = KiLockQuantumTarget + CycleTime;
        if (CycleTime > QuantumTarget || QuantumTarget < NewQuantumTarget)
            Thread->QuantumTarget = NewQuantumTarget;

        /* Release the lock */
        KiReleaseThreadLock(Thread);

        /* Make the thread ready */
        KiDeferredReadyThread(Thread);
    }

    /* Update the queue index for next time */
    if ((Count) && (Number))
    {
        /* Reset the queue at index 1 */
        Prcb->QueueIndex = 1;
    }
    else
    {
        /* Set the index we're in now */
        Prcb->QueueIndex = Index;
    }

    /* Increment the CPU number for next time and normalize to CPU count */
    ScanIndex++;
    if (ScanIndex == KeNumberProcessors) ScanIndex = 0;

    /* Return the index */
    *ScanLast = ScanIndex;
}

VOID
NTAPI
KeBalanceSetManager(IN PVOID Context)
{
    KDPC ScanDpc;
    KTIMER PeriodTimer;
    LARGE_INTEGER DueTime;
    KWAIT_BLOCK WaitBlockArray[1];
    PVOID WaitObjects[1];
    NTSTATUS Status;

    /* Set us at a low real-time priority level */
    KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY);

    /* Setup the timer and scanner DPC */
    KeInitializeTimerEx(&PeriodTimer, SynchronizationTimer);
    KeInitializeDpc(&ScanDpc, KiScanReadyQueues, &KiReadyScanLast);

    /* Setup the periodic timer */
    DueTime.QuadPart = -1 * 10 * 1000 * 1000;
    KeSetTimerEx(&PeriodTimer, DueTime, 1000, &ScanDpc);

    /* Setup the wait objects */
    WaitObjects[0] = &PeriodTimer;
    //WaitObjects[1] = MmWorkingSetManagerEvent; // NO WS Management Yet!

    /* Start wait loop */
    do
    {
        /* Wait on our objects */
        Status = KeWaitForMultipleObjects(1,
                                          WaitObjects,
                                          WaitAny,
                                          Executive,
                                          KernelMode,
                                          FALSE,
                                          NULL,
                                          WaitBlockArray);
        switch (Status)
        {
            /* Check if our timer expired */
            case STATUS_WAIT_0:

                /* Adjust lookaside lists */
                //ExAdjustLookasideDepth();

                /* Call the working set manager */
                //MmWorkingSetManager();

                /* FIXME: Outswap stacks */

                /* Done */
                break;

            /* Check if the working set manager notified us */
            case STATUS_WAIT_1:

                /* Call the working set manager */
                //MmWorkingSetManager();
                break;

            /* Anything else */
            default:
                DPRINT1("BALMGR: Illegal wait status, %lx =\n", Status);
                break;
        }
    } while (TRUE);
}
