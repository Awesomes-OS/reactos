/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/thrdobj.c
 * PURPOSE:         Implements routines to manage the Kernel Thread Object
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

extern EX_WORK_QUEUE ExWorkerQueue[MaximumWorkQueue];
extern LIST_ENTRY PspReaperListHead;

/* FUNCTIONS *****************************************************************/

UCHAR
NTAPI
KeFindNextRightSetAffinity(IN UCHAR Number,
                           IN ULONG Set)
{
    ULONG Bit, Result;
    ASSERT(Set != 0);

    /* Calculate the mask */
    Bit = (AFFINITY_MASK(Number) - 1) & Set;

    /* If it's 0, use the one we got */
    if (!Bit) Bit = Set;

    /* Now find the right set and return it */
    BitScanReverse(&Result, Bit);
    return (UCHAR)Result;
}

BOOLEAN
NTAPI
KeReadStateThread(IN PKTHREAD Thread)
{
    ASSERT_THREAD(Thread);

    /* Return signal state */
    return (BOOLEAN)Thread->Header.SignalState;
}

KPRIORITY
NTAPI
KeQueryBasePriorityThread(IN PKTHREAD Thread)
{
    LONG BaseIncrement;
    PKPROCESS Process;
    KLOCK_QUEUE_HANDLE ProcessLock;
    ASSERT_THREAD(Thread);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Get the Process */
    Process = Thread->Process;

    /* Lock the process */
    KiAcquireProcessLockRaiseToSynch(Process, &ProcessLock);

    /* Lock the thread */
    KiAcquireThreadLock(Thread);

    /* Calculate the base increment */
    BaseIncrement = Thread->BasePriority - Process->BasePriority;

    /* If saturation occured, return the saturation increment instead */
    if (Thread->Saturation) BaseIncrement = (HIGH_PRIORITY + 1) / 2 *
                                            Thread->Saturation;

    /* Release thread lock */
    KiReleaseThreadLock(Thread);

    /* Release process lock */
    KiReleaseProcessLock(&ProcessLock);

    return BaseIncrement;
}

BOOLEAN
NTAPI
KeSetDisableBoostThread(IN OUT PKTHREAD Thread,
                        IN BOOLEAN Disable)
{
    ASSERT_THREAD(Thread);

    /* Check if we're enabling or disabling */
    if (Disable)
    {
        /* Set the bit */
        return InterlockedBitTestAndSet(&Thread->ThreadFlags, 1);
    }
    else
    {
        /* Remove the bit */
        return InterlockedBitTestAndReset(&Thread->ThreadFlags, 1);
    }
}

VOID
NTAPI
KeReadyThread(IN PKTHREAD Thread)
{
    KIRQL OldIrql;
    ASSERT_THREAD(Thread);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Lock the Dispatcher Database */
    OldIrql = KiAcquireDispatcherLock();

    /* Make the thread ready */
    KiReadyThread(Thread);

    /* Unlock dispatcher database */
    KiReleaseDispatcherLock(OldIrql);
}

ULONG
NTAPI
KeAlertResumeThread(IN PKTHREAD Thread)
{
    ULONG PreviousCount;
    ASSERT_THREAD(Thread);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    const KIRQL OldIrql = KeRaiseIrqlToSynchLevel();

    KeAlertThread(Thread, KernelMode);

    PreviousCount = KeResumeThread(Thread);

    KeLowerIrql(OldIrql);
    return PreviousCount;
}

BOOLEAN
NTAPI
KeAlertThread(IN PKTHREAD Thread,
              IN KPROCESSOR_MODE AlertMode)
{
    BOOLEAN PreviousState;
    KLOCK_QUEUE_HANDLE ApcLock;
    ASSERT_THREAD(Thread);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Lock the Dispatcher Database and the APC Queue */
    KiAcquireApcLockRaiseToSynch(Thread, &ApcLock);
    KiAcquireDispatcherLockAtSynchLevel();

    /* Save the Previous State */
    PreviousState = Thread->Alerted[AlertMode];

    /* Check if it's already alerted */
    if (!PreviousState)
    {
        /* Check if the thread is alertable, and blocked in the given mode */
        if ((Thread->State == Waiting) &&
            (Thread->Alertable) &&
            (AlertMode <= Thread->WaitMode))
        {
            /* Abort the wait to alert the thread */
            KiUnwaitThread(Thread, STATUS_ALERTED, THREAD_ALERT_INCREMENT);
        }
        else
        {
            /* Otherwise, merely set the alerted state */
            Thread->Alerted[AlertMode] = TRUE;
        }
    }

    /* Release the Dispatcher Lock */
    KiReleaseDispatcherLockFromSynchLevel();
    KiReleaseApcLockFromSynchLevel(&ApcLock);
    KiExitDispatcher(ApcLock.OldIrql);

    /* Return the old state */
    return PreviousState;
}

ULONG64
NTAPI
KiEndThreadCycleAccumulation(IN PKPRCB Prcb, IN PKTHREAD Thread, OUT ULONG64* CurrentClock OPTIONAL)
{
    ASSERT(Prcb == KeGetCurrentPrcb());

    ULONGLONG Counter = KeQueryPerformanceCounter(NULL).QuadPart;

    ULONGLONG NewCycles = Counter - Prcb->StartCycles;

    Prcb->CycleTime += NewCycles;
    Thread->CycleTime += NewCycles;

    Prcb->StartCycles = Counter;

    if (CurrentClock)
        *CurrentClock = Counter;

    return Prcb->CycleTime;
}

ULONG64
NTAPI
KiUpdateTotalCyclesCurrentThread(IN PKPRCB Prcb, IN PKTHREAD Thread, OUT ULONG64* CurrentClock OPTIONAL)
{
    ASSERT(Prcb);
    ASSERT(Thread);

    const ULONG64 Result = KiEndThreadCycleAccumulation(Prcb, Thread, CurrentClock);

    return Result;
}

VOID
NTAPI
KeBoostPriorityThread(IN PKTHREAD Thread,
                      IN KPRIORITY Increment)
{
    KIRQL OldIrql;
    KPRIORITY Priority;
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Lock the Dispatcher Database */
    OldIrql = KiAcquireDispatcherLock();

    const PKPRCB Prcb = KeGetCurrentPrcb();
    const BOOLEAN CurrentThread = Thread == Prcb->CurrentThread;

    /* Only threads in the dynamic range get boosts */
    if (Thread->Priority < LOW_REALTIME_PRIORITY)
    {
        /* Lock the thread */
        KiAcquireThreadLock(Thread);

        /* Check again, and make sure there's not already a boost */
        if ((Thread->Priority < LOW_REALTIME_PRIORITY) &&
            !(Thread->PriorityDecrement))
        {
            /* Compute the new priority and see if it's higher */
            Priority = Thread->BasePriority + Increment;
            if (Priority > Thread->Priority)
            {
                if (Priority >= LOW_REALTIME_PRIORITY)
                {
                    Priority = LOW_REALTIME_PRIORITY - 1;
                }

                KiSetQuantumTargetThread(Prcb, Thread, CurrentThread);

                /* Set the new Priority */
                KiSetPriorityThread(Thread, Priority);
            }
        }

        /* Release thread lock */
        KiReleaseThreadLock(Thread);
    }

    /* Release the dispatcher lock */
    KiReleaseDispatcherLock(OldIrql);
}

VOID
NTAPI
KiResumeThread(PKTHREAD Thread)
{
    /* Lock the dispatcher */
    KiAcquireDispatcherLockAtSynchLevel();

    /* Signal and satisfy */
    Thread->SuspendSemaphore.Header.SignalState++;
    KiWaitTest(&Thread->SuspendSemaphore.Header, IO_NO_INCREMENT);

    /* Release the dispatcher */
    KiReleaseDispatcherLockFromSynchLevel();
}

ULONG
NTAPI
KeForceResumeThread(IN PKTHREAD Thread)
{
    KLOCK_QUEUE_HANDLE ApcLock;
    ULONG PreviousCount;
    ASSERT_THREAD(Thread);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Lock the APC Queue */
    KiAcquireApcLockRaiseToSynch(Thread, &ApcLock);

    /* Save the old Suspend Count */
    PreviousCount = Thread->SuspendCount + Thread->FreezeCount;

    /* If the thread is suspended, wake it up!!! */
    if (PreviousCount)
    {
        /* Unwait it completely */
        Thread->SuspendCount = 0;
        Thread->FreezeCount = 0;

        KiResumeThread(Thread);
    }

    /* Release Lock and return the Old State */
    KiReleaseApcLockFromSynchLevel(&ApcLock);
    KiExitDispatcher(ApcLock.OldIrql);
    return PreviousCount;
}

#if (NTDDI_VERSION < NTDDI_WIN8)
VOID
NTAPI
KeFreezeAllThreads(VOID)
{
    KLOCK_QUEUE_HANDLE LockHandle, ApcLock;
    PKTHREAD Current, CurrentThread = KeGetCurrentThread();
    PKPROCESS Process = CurrentThread->ApcState.Process;
    PLIST_ENTRY ListHead, NextEntry;
    LONG OldCount;
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Lock the process */
    KiAcquireProcessLockRaiseToSynch(Process, &LockHandle);

    /* If someone is already trying to free us, try again */
    while (CurrentThread->FreezeCount)
    {
        /* Release and re-acquire the process lock so the APC will go through */
        KiReleaseProcessLock(&LockHandle);
        KiAcquireProcessLockRaiseToSynch(Process, &LockHandle);
    }

    /* Enter a critical region */
    KeEnterCriticalRegion();

    /* Loop the Process's Threads */
    ListHead = &Process->ThreadListHead;
    NextEntry = ListHead->Flink;
    do
    {
        /* Get the current thread */
        Current = CONTAINING_RECORD(NextEntry, KTHREAD, ThreadListEntry);

        /* Lock it */
        KiAcquireApcLockAtSynchLevel(Current, &ApcLock);

        /* Make sure it's not ours, and check if APCs are enabled */
        if ((Current != CurrentThread) && (Current->ApcQueueable))
        {
            /* Sanity check */
            OldCount = Current->SuspendCount;
            ASSERT(OldCount != MAXIMUM_SUSPEND_COUNT);

            /* Increase the freeze count */
            Current->FreezeCount++;

            /* Make sure it wasn't already suspended */
            if (!(OldCount) && !(Current->SuspendCount))
            {
                /* Did we already insert it? */
                if (!Current->SuspendApc.Inserted)
                {
                    /* Insert the APC */
                    Current->SuspendApc.Inserted = TRUE;
                    KiInsertQueueApc(&Current->SuspendApc);
                    KiSignalThreadForApc(KeGetCurrentPrcb(), &Current->SchedulerApc, IO_NO_INCREMENT, DISPATCH_LEVEL);
                }
                else
                {
                    /* Lock the dispatcher */
                    KiAcquireDispatcherLockAtSynchLevel();

                    /* Unsignal the semaphore, the APC was already inserted */
                    Current->SuspendSemaphore.Header.SignalState--;

                    /* Release the dispatcher */
                    KiReleaseDispatcherLockFromSynchLevel();
                }
            }
        }

        /* Release the APC lock */
        KiReleaseApcLockFromSynchLevel(&ApcLock);

        /* Move to the next thread */
        NextEntry = NextEntry->Flink;
    } while (NextEntry != ListHead);

    /* Release the process lock and exit the dispatcher */
    KiReleaseProcessLockFromSynchLevel(&LockHandle);
    KiExitDispatcher(LockHandle.OldIrql);
}
#else
BOOLEAN
NTAPI
KiFreezeSingleThread(PKPRCB Prcb, PKTHREAD Thread)
{
    ASSERT_THREAD(Thread);

    Thread->FreezeCount = 1;

    const BOOLEAN Success = KiSuspendThread(Thread, Prcb);
    if (!Success)
        Thread->FreezeCount = 0;

    return Success;
}

BOOLEAN
NTAPI
KeFreezeProcess(PKPROCESS Process, BOOLEAN DeepFreeze)
{
    KLOCK_QUEUE_HANDLE LockHandle;
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Lock the process */
    KiAcquireProcessLock(Process, &LockHandle);

    const PKPRCB Prcb = KeGetCurrentPrcb();

    const BOOLEAN FreezeNeeded = !Process->FreezeCount && !Process->DeepFreeze;

    if (DeepFreeze)
    {
        Process->DeepFreezeStartTime = KiQueryUnbiasedInterruptTime(FALSE);
        Process->DeepFreeze = TRUE;
    }
    else
    {
        Process->FreezeCount++;
    }

    if (FreezeNeeded)
    {
        PLIST_ENTRY ListHead, NextEntry;
        KLOCK_QUEUE_HANDLE ApcLock;

        /* Loop the Process's Threads */
        ListHead = &Process->ThreadListHead;
        NextEntry = ListHead->Flink;
        do
        {
            /* Get the current thread */
            PKTHREAD Current = CONTAINING_RECORD(NextEntry, KTHREAD, ThreadListEntry);

            /* Lock it */
            KiAcquireApcLockAtDpcLevel(Current, &ApcLock);

            if (DeepFreeze || !Current->BypassProcessFreeze)
            {
                KiFreezeSingleThread(Prcb, Current);
            }

            /* Release the APC lock */
            KiReleaseApcLockFromDpcLevel(&ApcLock);

            /* Move to the next thread */
            NextEntry = NextEntry->Flink;
        } while (NextEntry != ListHead);
    }

    /* Release the process lock and exit the dispatcher */
    KiReleaseProcessLockFromDpcLevel(&LockHandle);

    if (FreezeNeeded)
    {
        KiExitDispatcher(LockHandle.OldIrql);
    }
    else
    {
        KeLowerIrql(LockHandle.OldIrql);
    }

    return !FreezeNeeded;
}

BOOLEAN
NTAPI
KiThawSingleThread(PKPRCB Prcb, PKTHREAD Thread, BOOLEAN Force)
{
    BOOLEAN Success = TRUE;

    ASSERT_THREAD(Thread);

    if (Thread->FreezeCount || Force)
    {
        Thread->FreezeCount = FALSE;

        if (!Thread->SuspendCount)
        {
            Success = KiSuspendThread(Thread, Prcb);
        }
    }

    return Success;
}

VOID
NTAPI
KeForceResumeProcess(PKPROCESS Process)
{
    KLOCK_QUEUE_HANDLE LockHandle;
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Lock the process */
    KiAcquireProcessLock(Process, &LockHandle);

    const PKPRCB Prcb = KeGetCurrentPrcb();

    const BOOLEAN ActionNeeded = Process->FreezeCount || Process->DeepFreeze;

    if (ActionNeeded)
    {
        Process->DeepFreeze = FALSE;
        Process->FreezeCount = 0;

        PLIST_ENTRY ListHead, NextEntry;

        /* Loop the Process's Threads */
        ListHead = &Process->ThreadListHead;
        NextEntry = ListHead->Flink;
        do
        {
            /* Get the current thread */
            PKTHREAD Current = CONTAINING_RECORD(NextEntry, KTHREAD, ThreadListEntry);

            KiThawSingleThread(Prcb, Current, TRUE);

            /* Move to the next thread */
            NextEntry = NextEntry->Flink;
        } while (NextEntry != ListHead);
    }

    /* Release the process lock and exit the dispatcher */
    KiReleaseProcessLockFromDpcLevel(&LockHandle);

    if (ActionNeeded)
    {
        KiExitDispatcher(LockHandle.OldIrql);
    }
    else
    {
        KeLowerIrql(LockHandle.OldIrql);
    }
}

ULONG32
NTAPI
KeThawProcess(PKPROCESS Process, BOOLEAN DeepUnfreeze)
{
    KLOCK_QUEUE_HANDLE LockHandle;
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Lock the process */
    KiAcquireProcessLock(Process, &LockHandle);

    const PKPRCB Prcb = KeGetCurrentPrcb();

    const ULONG32 ActionNeeded = Process->FreezeCount + (Process->DeepFreeze ? 1 : 0);

    ULONG64 Bias = 0;

    if (ActionNeeded)
    {
        if (DeepUnfreeze)
        {
            Process->DeepFreeze = FALSE;
            Bias = KiQueryUnbiasedInterruptTime(FALSE) - Process->DeepFreezeStartTime;
        }
        else
        {
            Process->FreezeCount--;
        }
    }

    if (DeepUnfreeze && Process->TimerVirtualization && Bias)
    {
        PLIST_ENTRY ListHead, NextEntry;

        /* Loop the Process's Threads */
        ListHead = &Process->ThreadListHead;
        NextEntry = ListHead->Flink;
        do
        {
            /* Get the current thread */
            PKTHREAD Current = CONTAINING_RECORD(NextEntry, KTHREAD, ThreadListEntry);

            Current->RelativeTimerBias += Bias;

            /* Move to the next thread */
            NextEntry = NextEntry->Flink;
        } while (NextEntry != ListHead);
    }

    if (ActionNeeded == 1)
    {
        PLIST_ENTRY ListHead, NextEntry;

        /* Loop the Process's Threads */
        ListHead = &Process->ThreadListHead;
        NextEntry = ListHead->Flink;
        do
        {
            /* Get the current thread */
            PKTHREAD Current = CONTAINING_RECORD(NextEntry, KTHREAD, ThreadListEntry);

            if (DeepUnfreeze || !Current->BypassProcessFreeze)
            {
                KiThawSingleThread(Prcb, Current, FALSE);
            }

            /* Move to the next thread */
            NextEntry = NextEntry->Flink;
        } while (NextEntry != ListHead);
    }

    /* Release the process lock and exit the dispatcher */
    KiReleaseProcessLockFromDpcLevel(&LockHandle);

    if (ActionNeeded == 1)
    {
        KiExitDispatcher(LockHandle.OldIrql);
    }
    else
    {
        KeLowerIrql(LockHandle.OldIrql);
    }

    return ActionNeeded;
}
#endif

ULONG
NTAPI
KeResumeThread(IN PKTHREAD Thread)
{
    KLOCK_QUEUE_HANDLE ApcLock;
    ULONG PreviousCount;
    ASSERT_THREAD(Thread);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Lock the APC Queue */
    KiAcquireApcLockRaiseToSynch(Thread, &ApcLock);

    /* Save the Old Count */
    PreviousCount = Thread->SuspendCount;

    /* Check if it existed */
    if (PreviousCount)
    {
        /* Decrease the suspend count */
        Thread->SuspendCount--;

        /* Check if the thrad is still suspended or not */
        if ((!Thread->SuspendCount) && (!Thread->FreezeCount))
        {
            KiResumeThread(Thread);
        }
    }

    /* Release APC Queue lock and return the Old State */
    KiReleaseApcLockFromSynchLevel(&ApcLock);
    KiExitDispatcher(ApcLock.OldIrql);
    return PreviousCount;
}

VOID
NTAPI
KeRundownThread(VOID)
{
    KIRQL OldIrql;
    PKTHREAD Thread = KeGetCurrentThread();
    PLIST_ENTRY NextEntry, ListHead;
    PKMUTANT Mutant;
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Optimized path if nothing is on the list at the moment */
    if (IsListEmpty(&Thread->MutantListHead)) return;

    /* Lock the Dispatcher Database */
    OldIrql = KiAcquireDispatcherLock();

    /* Get the List Pointers */
    ListHead = &Thread->MutantListHead;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        /* Get the Mutant */
        Mutant = CONTAINING_RECORD(NextEntry, KMUTANT, MutantListEntry);
        ASSERT_MUTANT(Mutant);

        /* Make sure it's not terminating with APCs off */
        if (Mutant->ApcDisable)
        {
            /* Bugcheck the system */
            KeBugCheckEx(THREAD_TERMINATE_HELD_MUTEX,
                         (ULONG_PTR)Thread,
                         (ULONG_PTR)Mutant,
                         0,
                         0);
        }

        /* Now we can remove it */
        RemoveEntryList(&Mutant->MutantListEntry);

        /* Unconditionally abandon it */
        Mutant->Header.SignalState = 1;
        Mutant->Abandoned = TRUE;
        Mutant->OwnerThread = NULL;

        /* Check if the Wait List isn't empty */
        if (!IsListEmpty(&Mutant->Header.WaitListHead))
        {
            /* Wake the Mutant */
            KiWaitTest(&Mutant->Header, MUTANT_INCREMENT);
        }

        /* Move on */
        NextEntry = Thread->MutantListHead.Flink;
    }

    /* Release the Lock */
    KiReleaseDispatcherLock(OldIrql);
}

VOID
NTAPI
KeStartThread(IN OUT PKTHREAD Thread)
{
    KLOCK_QUEUE_HANDLE LockHandle;
#ifdef CONFIG_SMP
    PKNODE Node;
#endif
    UCHAR IdealProcessor = 0;
    PKPROCESS Process = Thread->ApcState.Process;
    const PKTHREAD CreationThread = KeGetCurrentThread();

    /* Setup static fields from parent */
    KiAssignThreadDisableBoostFlag(Thread, KiTestProcessDisableBoostFlag(Process));
#if defined(_M_IX86) && (NTDDI_VERSION < NTDDI_LONGHORN)
    Thread->Iopl = Process->Iopl;
#endif
    Thread->QuantumReset = Process->QuantumReset;
    Thread->QuantumTarget = KiCyclesPerClockQuantum * Process->QuantumReset;
    Thread->SystemAffinityActive = FALSE;

    KiVBoxPrint("KeStartThread 1\n");

    /* Lock the process */
    KiAcquireProcessLockRaiseToSynch(Process, &LockHandle);

    KiVBoxPrint("KeStartThread 2\n");

    /* Setup volatile data */
    Thread->Priority = Process->BasePriority;
    Thread->BasePriority = Process->BasePriority;

    if (CreationThread->Process == Process)
    {
        Thread->Affinity.Group = CreationThread->UserAffinity.Group;
        Thread->Affinity.Mask = Process->Affinity.Bitmap[Thread->Affinity.Group];
    }
    else
    {
        KeFirstGroupAffinityEx(&Thread->Affinity, &Process->Affinity);
    }

    KiVBoxPrint("KeStartThread 3\n");

    Thread->UserAffinity = Thread->Affinity;
    KiUpdateNodeAffinitizedFlag(Thread);

    KiVBoxPrint("KeStartThread 4\n");

#ifdef CONFIG_SMP
    UINT16 Seed = Process->ThreadSeed[Thread->Affinity.Group];

    /* Get the KNODE and its PRCB */
    Node = KeNodeBlock[Process->IdealNode[Thread->Affinity.Group]];
    //UINT16 ProcessIdealProcessor = Process->IdealProcessor[Thread->Affinity.Group];

    GROUP_AFFINITY IdealAffinity = Thread->Affinity;
    IdealAffinity.Mask &= Node->Affinity.Mask;

    /* Get the new thread seed */
    IdealProcessor = KeSelectIdealProcessor(Node, &IdealAffinity, &Seed);
    Process->ThreadSeed[Thread->Affinity.Group] = Seed;
#endif

    /* Set the Ideal Processor */
    Thread->IdealProcessor = IdealProcessor;
    Thread->UserIdealProcessor = IdealProcessor;

    /* Lock the Dispatcher Database */
    KiAcquireDispatcherLockAtSynchLevel();

    KiVBoxPrint("KeStartThread 5\n");

    /* Insert the thread into the process list */
    InsertTailList(&Process->ThreadListHead, &Thread->ThreadListEntry);

    /* Increase the stack count */
    ASSERT(Process->StackCount != MAXULONG_PTR);
    Process->StackCount++;

    KiVBoxPrint("KeStartThread 6\n");

    /* Release locks and return */
    KiReleaseDispatcherLockFromSynchLevel();

#if (NTDDI_VERSION >= NTDDI_WIN8)
    if (Process->DeepFreeze)
        KiFreezeSingleThread(KeGetCurrentPrcb(), Thread);
#endif

    KiReleaseProcessLock(&LockHandle);

    KiVBoxPrint("KeStartThread EXIT\n");
}

VOID
NTAPI
KiSchedulerRundown(IN PKAPC Apc)
{
    /* Does nothing */
    UNREFERENCED_PARAMETER(Apc);
}

VOID
NTAPI
KiSchedulerNop(IN PKAPC Apc,
             IN PKNORMAL_ROUTINE *NormalRoutine,
             IN PVOID *NormalContext,
             IN PVOID *SystemArgument1,
             IN PVOID *SystemArgument2)
{
    /* Does nothing */
    UNREFERENCED_PARAMETER(Apc);
    UNREFERENCED_PARAMETER(NormalRoutine);
    UNREFERENCED_PARAMETER(NormalContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);
}

VOID
NTAPI
KiSchedulerApc(IN PVOID NormalContext,
                IN PVOID SystemArgument1,
                IN PVOID SystemArgument2)
{
    /* Non-alertable kernel-mode suspended wait */
    KeWaitForSingleObject(&KeGetCurrentThread()->SuspendSemaphore,
                          Suspended,
                          KernelMode,
                          FALSE,
                          NULL);
}

BOOLEAN
FASTCALL
KiSuspendThread(PKTHREAD Thread, PKPRCB Prcb)
{
    /* Should we bother to queue at all? */
    if (!Thread->ApcQueueable)
        return FALSE;

    /* Check if we should suspend it */
    if (TRUE)
    {
        /* Is the APC already inserted? */
        if (!Thread->SchedulerApc.Inserted)
        {
            /* Not inserted, insert it */
            Thread->SchedulerApc.Inserted = TRUE;
            KiInsertQueueApc(&Thread->SchedulerApc);
            KiSignalThreadForApc(Prcb, &Thread->SchedulerApc, IO_NO_INCREMENT, DISPATCH_LEVEL);
        }
        else
        {
            /* Lock the dispatcher */
            KiAcquireDispatcherLockAtSynchLevel();

            /* Unsignal the semaphore, the APC was already inserted */
            Thread->SuspendSemaphore.Header.SignalState--;

            /* Release the dispatcher */
            KiReleaseDispatcherLockFromSynchLevel();
        }

        return TRUE;
    }
}

ULONG
NTAPI
KeSuspendThread(PKTHREAD Thread)
{
    KLOCK_QUEUE_HANDLE ApcLock;
    ULONG PreviousCount;
    ASSERT_THREAD(Thread);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    const PKPRCB Prcb = KeGetCurrentPrcb();

    /* Lock the APC Queue */
    KiAcquireApcLockRaiseToSynch(Thread, &ApcLock);

    /* Save the Old Count */
    PreviousCount = Thread->SuspendCount;

    /* Handle the maximum */
    if (PreviousCount == MAXIMUM_SUSPEND_COUNT)
    {
        /* Raise an exception */
        KiReleaseApcLock(&ApcLock);
        RtlRaiseStatus(STATUS_SUSPEND_COUNT_EXCEEDED);
    }

    /* Increment the suspend count */
    Thread->SuspendCount++;

    if (!KiSuspendThread(Thread, Prcb))
        Thread->SuspendCount--;

    /* Release Lock and return the Old State */
    KiReleaseApcLockFromSynchLevel(&ApcLock);
    KiExitDispatcher(ApcLock.OldIrql);
    return PreviousCount;
}

#if (NTDDI_VERSION < NTDDI_WIN8)
VOID
NTAPI
KeThawAllThreads(VOID)
{
    KLOCK_QUEUE_HANDLE LockHandle, ApcLock;
    PKTHREAD Current, CurrentThread = KeGetCurrentThread();
    PKPROCESS Process = CurrentThread->ApcState.Process;
    PLIST_ENTRY ListHead, NextEntry;
    LONG OldCount;
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Lock the process */
    KiAcquireProcessLockRaiseToSynch(Process, &LockHandle);

    /* Loop the Process's Threads */
    ListHead = &Process->ThreadListHead;
    NextEntry = ListHead->Flink;
    do
    {
        /* Get the current thread */
        Current = CONTAINING_RECORD(NextEntry, KTHREAD, ThreadListEntry);

        /* Lock it */
        KiAcquireApcLockAtSynchLevel(Current, &ApcLock);

        /* Make sure we are frozen */
        OldCount = Current->FreezeCount;
        if (OldCount)
        {
            /* Decrease the freeze count */
            Current->FreezeCount--;

            /* Check if both counts are zero now */
            if (!(Current->SuspendCount) && (!Current->FreezeCount))
            {
                /* Lock the dispatcher */
                KiAcquireDispatcherLockAtSynchLevel();

                /* Signal the suspend semaphore and wake it */
                Current->SuspendSemaphore.Header.SignalState++;
                KiWaitTest(&Current->SuspendSemaphore, 0);

                /* Unlock the dispatcher */
                KiReleaseDispatcherLockFromSynchLevel();
            }
        }

        /* Release the APC lock */
        KiReleaseApcLockFromSynchLevel(&ApcLock);

        /* Go to the next one */
        NextEntry = NextEntry->Flink;
    } while (NextEntry != ListHead);

    /* Release the process lock and exit the dispatcher */
    KiReleaseProcessLockFromSynchLevel(&LockHandle);
    KiExitDispatcher(LockHandle.OldIrql);

    /* Leave the critical region */
    KeLeaveCriticalRegion();
}
#endif

BOOLEAN
NTAPI
KeTestAlertThread(IN KPROCESSOR_MODE AlertMode)
{
    PKTHREAD Thread = KeGetCurrentThread();
    BOOLEAN OldState;
    KLOCK_QUEUE_HANDLE ApcLock;
    ASSERT_THREAD(Thread);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Lock the Dispatcher Database and the APC Queue */
    KiAcquireApcLockRaiseToSynch(Thread, &ApcLock);

    /* Save the old State */
    OldState = Thread->Alerted[AlertMode];

    /* Check the Thread is alerted */
    if (OldState)
    {
        /* Disable alert for this mode */
        Thread->Alerted[AlertMode] = FALSE;
    }
    else if ((AlertMode != KernelMode) &&
             (!IsListEmpty(&Thread->ApcState.ApcListHead[UserMode])))
    {
        /* If the mode is User and the Queue isn't empty, set Pending */
        Thread->ApcState.UserApcPending = TRUE;
    }

    /* Release Locks and return the Old State */
    KiReleaseApcLock(&ApcLock);
    return OldState;
}

NTSTATUS
NTAPI
KeInitThread(IN OUT PKTHREAD Thread,
             IN PVOID KernelStack,
             IN PKSYSTEM_ROUTINE SystemRoutine,
             IN PKSTART_ROUTINE StartRoutine,
             IN PVOID StartContext,
             IN PCONTEXT Context,
             IN PVOID Teb,
             IN PKPROCESS Process)
{
    BOOLEAN AllocatedStack = FALSE;
    ULONG i;
    PKWAIT_BLOCK TimerWaitBlock;
    PKTIMER Timer;
    NTSTATUS Status;

    /* Initialize the Dispatcher Header */
    Thread->Header.Type = ThreadObject;
    Thread->Header.ThreadControlFlags = 0;
    Thread->Header.DebugActive = FALSE;
    Thread->Header.SignalState = 0;
    InitializeListHead(&(Thread->Header.WaitListHead));

    /* Initialize the Mutant List */
    InitializeListHead(&Thread->MutantListHead);

    /* Initialize the wait blocks */
    for (i = 0; i< (THREAD_WAIT_OBJECTS + 1); i++)
    {
        /* Put our pointer */
        Thread->WaitBlock[i].Thread = Thread;
    }

    /* Set swap settings */
    Thread->EnableStackSwap = TRUE;
    Thread->IdealProcessor = 1;
    Thread->SwapBusy = FALSE;
    Thread->KernelStackResident = TRUE;
    Thread->AdjustReason = AdjustNone;

    /* Initialize the lock */
    KeInitializeSpinLock(&Thread->ThreadLock);

    /* Setup the Service Descriptor Table for Native Calls */
    Thread->ServiceTable = KeServiceDescriptorTable;

    /* Setup APC Fields */
    InitializeListHead(&Thread->ApcState.ApcListHead[KernelMode]);
    InitializeListHead(&Thread->ApcState.ApcListHead[UserMode]);
    Thread->ApcState.Process = Process;
    Thread->ApcStatePointer[OriginalApcEnvironment] = &Thread->ApcState;
    Thread->ApcStatePointer[AttachedApcEnvironment] = &Thread->SavedApcState;
    Thread->ApcStateIndex = OriginalApcEnvironment;
    Thread->ApcQueueable = TRUE;

    if (!Context)
    {
        KiSetThreadSystemThreadFlagAssert(Thread);
    }

    KeInitializeSpinLock(&Thread->ApcQueueLock);

    /* Initialize the Suspend APC */
    KeInitializeApc(&Thread->SchedulerApc,
                    Thread,
                    OriginalApcEnvironment,
                    KiSchedulerNop,
                    KiSchedulerRundown,
                    KiSchedulerApc,
                    KernelMode,
                    NULL);

    /* Initialize the Suspend Semaphore */
    KeInitializeSemaphore(&Thread->SuspendSemaphore, 0, 2);

    /* Setup the timer */
    Timer = &Thread->Timer;
    KeInitializeTimer(Timer);
    TimerWaitBlock = &Thread->WaitBlock[TIMER_WAIT_BLOCK];
    TimerWaitBlock->Object = Timer;
    TimerWaitBlock->WaitKey = STATUS_TIMEOUT;
    TimerWaitBlock->WaitType = WaitAny;
    TimerWaitBlock->NextWaitBlock = NULL;

    /* Link the two wait lists together */
    TimerWaitBlock->WaitListEntry.Flink = &Timer->Header.WaitListHead;
    TimerWaitBlock->WaitListEntry.Blink = &Timer->Header.WaitListHead;

    /* Set the TEB and process */
    Thread->Teb = Teb;
    Thread->Process = Process;

    /* Check if we have a kernel stack */
    if (!KernelStack)
    {
        /* We don't, allocate one */
        KernelStack = MmCreateKernelStack(FALSE, 0);
        if (!KernelStack) return STATUS_INSUFFICIENT_RESOURCES;

        /* Remember for later */
        AllocatedStack = TRUE;
    }

    /* Set the Thread Stacks */
    Thread->InitialStack = KernelStack;
    Thread->StackBase = KernelStack;
    Thread->StackLimit = (ULONG_PTR)KernelStack - KERNEL_STACK_SIZE;
    Thread->KernelStackResident = TRUE;

    /* Enter SEH to avoid crashes due to user mode */
    Status = STATUS_SUCCESS;
    _SEH2_TRY
    {
        /* Initialize the Thread Context */
        KiInitializeContextThread(Thread,
                                  SystemRoutine,
                                  StartRoutine,
                                  StartContext,
                                  Context);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Set failure status */
        Status = STATUS_UNSUCCESSFUL;

        /* Check if a stack was allocated */
        if (AllocatedStack)
        {
            /* Delete the stack */
            MmDeleteKernelStack((PVOID)Thread->StackBase, FALSE);
            Thread->InitialStack = NULL;
        }
    }
    _SEH2_END;

    /* Set the Thread to initialized */
    Thread->State = Initialized;
    return Status;
}

VOID
NTAPI
KeInitializeThread(IN PKPROCESS Process,
                   IN OUT PKTHREAD Thread,
                   IN PKSYSTEM_ROUTINE SystemRoutine,
                   IN PKSTART_ROUTINE StartRoutine,
                   IN PVOID StartContext,
                   IN PCONTEXT Context,
                   IN PVOID Teb,
                   IN PVOID KernelStack)
{
    /* Initialize and start the thread on success */
    if (NT_SUCCESS(KeInitThread(Thread,
                                KernelStack,
                                SystemRoutine,
                                StartRoutine,
                                StartContext,
                                Context,
                                Teb,
                                Process)))
    {
        /* Start it */
        KeStartThread(Thread);
    }
}

VOID
NTAPI
KeUninitThread(IN PKTHREAD Thread)
{
    /* Delete the stack */
    MmDeleteKernelStack((PVOID)Thread->StackBase, FALSE);
    Thread->InitialStack = NULL;
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @unimplemented
 */
VOID
NTAPI
KeCapturePersistentThreadState(IN PVOID CurrentThread,
                               IN ULONG Setting1,
                               IN ULONG Setting2,
                               IN ULONG Setting3,
                               IN ULONG Setting4,
                               IN ULONG Setting5,
                               IN PVOID ThreadState)
{
    UNIMPLEMENTED;
}

/*
 * @implemented
 */
#undef KeGetCurrentThread
PKTHREAD
NTAPI
KeGetCurrentThread(VOID)
{
    /* Return the current thread on this PCR */
    return _KeGetCurrentThread();
}

/*
 * @implemented
 */
#undef KeGetPreviousMode
UCHAR
NTAPI
KeGetPreviousMode(VOID)
{
    /* Return the previous mode of this thread */
    return _KeGetPreviousMode();
}

/*
 * @implemented
 */
ULONG
NTAPI
KeQueryRuntimeThread(IN PKTHREAD Thread,
                     OUT PULONG UserTime)
{
    ASSERT_THREAD(Thread);

    /* Return the User Time */
    *UserTime = Thread->UserTime;

    /* Return the Kernel Time */
    return Thread->KernelTime;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
KeSetKernelStackSwapEnable(IN BOOLEAN Enable)
{
    BOOLEAN PreviousState;
    PKTHREAD Thread = KeGetCurrentThread();

    /* Save Old State */
    PreviousState = Thread->EnableStackSwap;

    /* Set New State */
    Thread->EnableStackSwap = Enable;

    /* Return Old State */
    return PreviousState;
}

/*
 * @implemented
 */
KPRIORITY
NTAPI
KeQueryPriorityThread(IN PKTHREAD Thread)
{
    ASSERT_THREAD(Thread);

    /* Return the current priority */
    return Thread->Priority;
}

VOID
FASTCALL
KiSetSystemAffinityThread(PKPRCB Prcb, const GROUP_AFFINITY* const GroupAffinity, ULONG32 TargetIdealProcessor)
{
    PKTHREAD CurrentThread = Prcb->CurrentThread;
    PKPRCB TargetPrcb;

    ASSERT(KeGetCurrentPrcb() == Prcb);
    ASSERT((GroupAffinity->Mask & KeQueryGroupAffinity(GroupAffinity->Group)) == GroupAffinity->Mask);

    /* Restore the affinity and enable system affinity */
    CurrentThread->Affinity.Group = GroupAffinity->Group;
    CurrentThread->Affinity.Mask = GroupAffinity->Mask;

    if (TargetIdealProcessor < MAX_PROC_GROUPS * MAXIMUM_PROC_PER_GROUP)
    {
        CurrentThread->IdealProcessor = TargetIdealProcessor;
        TargetPrcb = KiProcessorBlock[TargetIdealProcessor];
    }
    else
    {
        TargetPrcb = KiProcessorBlock[CurrentThread->IdealProcessor];

        /* Check if the ideal processor is part of the affinity */
        if (!KiPrcbInGroupAffinity(TargetPrcb, GroupAffinity))
        {
            const PKNODE Node = TargetPrcb->ParentNode;

            /* Calculate the affinity set */
            GROUP_AFFINITY Intersection = *GroupAffinity;

            if (Intersection.Group == Node->Affinity.Group)
            {
                const KAFFINITY MaskSetIntersection = Intersection.Mask & Node->Affinity.Mask;
                if (MaskSetIntersection)
                    Intersection.Mask = MaskSetIntersection;
            }

            /* Calculate the ideal CPU from the affinity set */
            const ULONG32 Index = KeFindFirstSetLeftGroupAffinity(&Intersection);

            CurrentThread->IdealProcessor = Index;
            TargetPrcb = KiProcessorBlock[Index];
        }
    }

    if (CurrentThread->SystemAffinityActive)
    {
        UNREFERENCED_LOCAL_VARIABLE(TargetPrcb);
        KiUpdateNodeAffinitizedFlag(CurrentThread);
    }

    /* Get the current PRCB and check if it doesn't match this affinity */
    if (!KiPrcbInGroupAffinity(Prcb, &CurrentThread->Affinity))
    {
        KiSetThreadForceDeferScheduleFlagAssert(CurrentThread);

        /* Check if there's no next thread scheduled */
        if (!Prcb->NextThread)
        {
            /* Lock the PRCB */
            KiAcquirePrcbLock(Prcb);

            if (!Prcb->NextThread)
            {
                /* Select a new standby thread */
                KiSelectNextThread(Prcb);
            }

            /* Release the PRCB lock */
            KiReleasePrcbLock(Prcb);
        }
    }
}

/*
 * @implemented
 */
VOID
NTAPI
KeRevertToUserAffinityThread(VOID)
{
    GROUP_AFFINITY GroupAffinity = { 0 };

    ASSERT(KeGetCurrentThread()->SystemAffinityActive);

    KeRevertToUserGroupAffinityThread(&GroupAffinity);
}

/*
 * @implemented
 */
VOID
NTAPI
KeRevertToUserAffinityThreadEx(IN KAFFINITY Affinity)
{
    GROUP_AFFINITY GroupAffinity = { 0 };

    GroupAffinity.Mask = Affinity;
    GroupAffinity.Group = KeForceGroupAwareness ? KeQueryActiveGroupCount() - 1 : 0;

    KeRevertToUserGroupAffinityThread(&GroupAffinity);
}

/*
 * @implemented
 */
VOID
NTAPI
KeRevertToUserGroupAffinityThread(IN PGROUP_AFFINITY PreviousAffinity)
{
    KIRQL OldIrql;
    PKPRCB Prcb;
    PKTHREAD CurrentThread = KeGetCurrentThread();

    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    if (CurrentThread->SystemAffinityActive && KiVerifyReservedFieldGroupAffinity(PreviousAffinity))
    {
        BOOLEAN RevertNeeded = !PreviousAffinity->Mask;

        if (!RevertNeeded)
        {
            PreviousAffinity->Mask &= KeQueryGroupAffinity(PreviousAffinity->Group);
            RevertNeeded = PreviousAffinity->Mask != 0;
        }

        if (RevertNeeded)
        {
            ULONG32 TargetIdealProcessor;
            PGROUP_AFFINITY NewAffinity;
            KLOCK_QUEUE_HANDLE ThreadLock;

            // Lock the Dispatcher Database and the thread
            OldIrql = KiAcquireDispatcherLock();
            KiAcquireApcLockAtSynchLevel(CurrentThread, &ThreadLock);

            Prcb = KeGetCurrentPrcb();

            if (PreviousAffinity->Mask)
            {
                // Use the previous affinity and prefer to keep using current processor, if possible
                TargetIdealProcessor = MAX_PROC_GROUPS * MAXIMUM_PROC_PER_GROUP;
                NewAffinity = PreviousAffinity;
            }
            else
            {
                // Set the user affinity and processor and disable system affinity
                NewAffinity = &CurrentThread->UserAffinity;
                TargetIdealProcessor = CurrentThread->UserIdealProcessor;
                CurrentThread->SystemAffinityActive = FALSE;
            }

            KiSetSystemAffinityThread(Prcb, NewAffinity, TargetIdealProcessor);

            // Release the locks
            KiReleaseApcLockFromSynchLevel(&ThreadLock);
            KiReleaseDispatcherLock(OldIrql);
        }
    }
}

NTSTATUS
FASTCALL
KeSetIdealProcessorThreadEx(PKTHREAD Thread, int ProcessorIndex, ULONG* OldIdealProcessor)
{
    ULONG OldIdealProcessorValue;

    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    const PKPRCB Prcb = KiProcessorBlock[ProcessorIndex];

    /* Lock the Dispatcher Database */
    const KIRQL oldIrql = KiAcquireDispatcherLock();

    if (Thread != KeGetCurrentThread() && Thread->SystemAffinityActive)
    {
        OldIdealProcessorValue = Thread->UserIdealProcessor;

        if (KiPrcbInGroupAffinity(Prcb, &Thread->UserAffinity))
        {
            Status = STATUS_SUCCESS;
            Thread->UserIdealProcessor = ProcessorIndex;
        }
    }
    else
    {
        OldIdealProcessorValue = Thread->IdealProcessor;

        if (KiPrcbInGroupAffinity(Prcb, &Thread->Affinity))
        {
            Thread->IdealProcessor = ProcessorIndex;

            if (!Thread->SystemAffinityActive)
                Thread->UserIdealProcessor = ProcessorIndex;

            Status = STATUS_SUCCESS;
        }
    }

    /* Release dispatcher lock */
    KiReleaseDispatcherLock(oldIrql);

    if (OldIdealProcessor)
        *OldIdealProcessor = OldIdealProcessorValue;

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
KeSetIdealProcessorThreadByNumber(IN PKTHREAD Thread,
                                  IN PPROCESSOR_NUMBER ProcessorNumber,
                                  IN PPROCESSOR_NUMBER OldIdealProcessorNumber)
{
    NTSTATUS status;

    /* Save Old Ideal Processor */
    ULONG OldIdealProcessor = INVALID_PROCESSOR_INDEX;

    const ULONG ProcessorIndex = KeGetProcessorIndexFromNumber(ProcessorNumber);

    if (ProcessorIndex == INVALID_PROCESSOR_INDEX)
    {
        status = STATUS_INVALID_PARAMETER;

        OldIdealProcessor = (Thread == KeGetCurrentThread()) ? Thread->IdealProcessor : Thread->UserIdealProcessor;
    }
    else
    {
        status = KeSetIdealProcessorThreadEx(Thread, ProcessorIndex, &OldIdealProcessor);
    }

    ASSERT(OldIdealProcessor != INVALID_PROCESSOR_INDEX);

    ASSERT(NT_SUCCESS(KeGetProcessorNumberFromIndex(OldIdealProcessor, OldIdealProcessorNumber)));

    return status;
}

/*
 * @implemented
 */
UCHAR
NTAPI
KeSetIdealProcessorThread(IN PKTHREAD Thread,
                          IN UCHAR Processor)
{
    PROCESSOR_NUMBER ProcessorNumber = {0}, OldIdealProcessor = {0};

    ProcessorNumber.Number = Processor;
    ProcessorNumber.Group = (Thread == KeGetCurrentThread()) ? Thread->Affinity.Group : Thread->UserAffinity.Group;

    KeSetIdealProcessorThreadByNumber(Thread, &ProcessorNumber, &OldIdealProcessor);

    return OldIdealProcessor.Number;
}

/*
 * @implemented
 */
KAFFINITY
NTAPI
KeSetSystemAffinityThreadEx(IN KAFFINITY Affinity)
{
    GROUP_AFFINITY GroupAffinity = {0};
    GROUP_AFFINITY OldAffinity = {0};

    GroupAffinity.Mask = Affinity;
    GroupAffinity.Group = KeForceGroupAwareness ? KeQueryActiveGroupCount() - 1 : 0;

    KeSetSystemGroupAffinityThread(&GroupAffinity, &OldAffinity);

    return OldAffinity.Mask;
}

/*
 * @implemented
 */
VOID
NTAPI
KeSetSystemAffinityThread(IN KAFFINITY Affinity)
{
    KeSetSystemAffinityThreadEx(Affinity);
}

/*
 * @implemented
 */
VOID
NTAPI
KeSetSystemGroupAffinityThread(IN PGROUP_AFFINITY Affinity, IN OUT PGROUP_AFFINITY OldAffinity)
{
    KIRQL OldIrql;
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    const BOOLEAN CorrectAffinity = KeQueryGroupAffinity(Affinity->Group) & Affinity->Mask
                                    && KiVerifyReservedFieldGroupAffinity(Affinity);

    /* Lock the Dispatcher Database */
    OldIrql = KiAcquireDispatcherLock();

    const PKPRCB Prcb = KeGetCurrentPrcb();
    const PKTHREAD CurrentThread = Prcb->CurrentThread;

    if (CurrentThread->SystemAffinityActive)
    {
        OldAffinity->Group = CurrentThread->Affinity.Group;
        OldAffinity->Mask = CurrentThread->Affinity.Mask;
    }
    else
    {
        CurrentThread->SystemAffinityActive = TRUE;
        RtlZeroMemory(OldAffinity, sizeof(*OldAffinity));
    }

    if (CorrectAffinity)
    {
        Affinity->Mask &= KeActiveProcessors.Bitmap[Affinity->Group];

        KiSetSystemAffinityThread(Prcb, Affinity, MAX_PROC_GROUPS * MAXIMUM_PROC_PER_GROUP);
    }

    /* Unlock dispatcher database */
    KiReleaseDispatcherLock(OldIrql);
}

/*
 * @implemented
 */
LONG
NTAPI
KeSetBasePriorityThread(IN PKTHREAD Thread,
                        IN LONG Increment)
{
    KIRQL OldIrql;
    KPRIORITY OldBasePriority, Priority, BasePriority;
    LONG OldIncrement;
    PKPROCESS Process;
    ASSERT_THREAD(Thread);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Get the process */
    Process = Thread->ApcState.Process;

    /* Lock the Dispatcher Database */
    OldIrql = KiAcquireDispatcherLock();

    const PKPRCB Prcb = KeGetCurrentPrcb();
    const BOOLEAN CurrentThread = Thread == Prcb->CurrentThread;

    /* Lock the thread */
    KiAcquireThreadLock(Thread);

    /* Save the old base priority and increment */
    OldBasePriority = Thread->BasePriority;
    OldIncrement = OldBasePriority - Process->BasePriority;

    /* If priority saturation happened, use the saturated increment */
    if (Thread->Saturation) OldIncrement = (HIGH_PRIORITY + 1) / 2 *
                                            Thread->Saturation;

    /* Reset the saturation value */
    Thread->Saturation = 0;

    /* Now check if saturation is being used for the new value */
    if (abs(Increment) >= ((HIGH_PRIORITY + 1) / 2))
    {
        /* Check if we need positive or negative saturation */
        Thread->Saturation = (Increment > 0) ? 1 : -1;
    }

    /* Normalize the Base Priority */
    BasePriority = Process->BasePriority + Increment;
    if (Process->BasePriority >= LOW_REALTIME_PRIORITY)
    {
        /* Check if it's too low */
        if (BasePriority < LOW_REALTIME_PRIORITY)
        {
            /* Set it to the lowest real time level */
            BasePriority = LOW_REALTIME_PRIORITY;
        }

        /* Check if it's too high */
        if (BasePriority > HIGH_PRIORITY) BasePriority = HIGH_PRIORITY;

        /* We are at real time, so use the raw base priority */
        Priority = BasePriority;
    }
    else
    {
        /* Check if it's entering the real time range */
        if (BasePriority >= LOW_REALTIME_PRIORITY)
        {
            /* Set it to the highest dynamic level */
            BasePriority = LOW_REALTIME_PRIORITY - 1;
        }

        /* Check if it's too low and normalize it */
        if (BasePriority <= LOW_PRIORITY) BasePriority = 1;

        /* Check if Saturation is used */
        if (Thread->Saturation)
        {
            /* Use the raw base priority */
            Priority = BasePriority;
        }
        else
        {
            /* Otherwise, calculate the new priority */
            Priority = KiComputeNewPriority(Thread, 0);
            Priority += (BasePriority - OldBasePriority);

            /* Check if it entered the real-time range */
            if (Priority >= LOW_REALTIME_PRIORITY)
            {
                /* Normalize it down to the highest dynamic priority */
                Priority = LOW_REALTIME_PRIORITY - 1;
            }
            else if (Priority <= LOW_PRIORITY)
            {
                /* It went too low, normalize it */
                Priority = 1;
            }
        }
    }

    /* Finally set the new base priority */
    Thread->BasePriority = (SCHAR)BasePriority;

    /* Reset the decrements */
    Thread->PriorityDecrement = 0;

    /* Check if we're changing priority after all */
    if (Priority != Thread->Priority)
    {
        /* Do the actual priority modification */
        KiSetQuantumTargetThread(Prcb, Thread, CurrentThread);
        KiSetPriorityThread(Thread, Priority);
    }

    /* Release thread lock */
    KiReleaseThreadLock(Thread);

    /* Release the dispatcher database and return old increment */
    KiReleaseDispatcherLock(OldIrql);
    return OldIncrement;
}

/*
 * @implemented
 */
VOID
NTAPI
KeQueryAffinityThread(IN PKTHREAD Thread,
    OUT PGROUP_AFFINITY Affinity)
{
    KIRQL OldIrql;
    GROUP_AFFINITY affinity;
    ASSERT_THREAD(Thread);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Lock the dispatcher database */
    OldIrql = KiAcquireDispatcherLock();

    affinity = Thread->UserAffinity;

    /* Release the dispatcher database */
    KiReleaseDispatcherLock(OldIrql);

    *Affinity = affinity;
}

/*
 * @implemented
 */
VOID
NTAPI
KeSetAffinityThread(IN PKTHREAD Thread,
                    IN PGROUP_AFFINITY Affinity)
{
    ASSERT(Affinity);
    ASSERT_THREAD(Thread);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Lock the dispatcher database */
    const KIRQL OldIrql = KiAcquireDispatcherLock();

    const PKPROCESS Process = Thread->Process;

    const KAFFINITY Mask = Process->Affinity.Bitmap[Affinity->Group];

    if (!Mask || (Mask & Affinity->Mask) != Affinity->Mask)
        KiExtendProcessAffinity(Process, Affinity->Group);

    if (!Affinity->Mask)
        Affinity->Mask = Process->Affinity.Bitmap[Affinity->Group];

    /* Call the internal function */
    KiSetAffinityThread(Thread, Affinity);

    /* Release the dispatcher database and return old affinity */
    KiReleaseDispatcherLock(OldIrql);
}

ULARGE_INTEGER
NTAPI
KiCaptureThreadCycleTime(IN PKTHREAD Thread)
{
    ASSERT_THREAD(Thread);

    ULARGE_INTEGER CycleTime;

    volatile ULONGLONG* CycleTimePtr = &Thread->CycleTime;
    volatile ULONG* HighCycleTimePtr = &Thread->HighCycleTime;

#ifdef _WIN64
    CycleTime.QuadPart = *CycleTimePtr;
#else
    do
    {
        CycleTime.QuadPart = *CycleTimePtr;

        if (CycleTime.HighPart == *HighCycleTimePtr)
            break;

        YieldProcessor();
    } while (TRUE);
#endif

    return CycleTime;
}

void
NTAPI
KiSetQuantumTargetThread(IN PKPRCB Prcb, IN PKTHREAD Thread, IN BOOLEAN UpdateCurrentThread)
{
    ASSERT_THREAD(Thread);
    ASSERT_IRQL_EQUAL(DISPATCH_LEVEL);
    ASSERT(!UpdateCurrentThread || Thread == KeGetCurrentThread());
    ASSERT(Prcb == KeGetCurrentPrcb());

     ULARGE_INTEGER CycleTime;

    if (UpdateCurrentThread)
    {
        ASSERT(Prcb == KeGetCurrentPrcb());

        if (Prcb->NestingLevel)
        {
            ASSERT_IRQL_GREATER_OR_EQUAL(DISPATCH_LEVEL);

            CycleTime.QuadPart = Thread->CycleTime;
        }
        else
        {
            _disable();

            CycleTime.QuadPart = KiUpdateTotalCyclesCurrentThread(Prcb, Thread, NULL);

            _enable();
        }
    }
    else
    {
        CycleTime = KiCaptureThreadCycleTime(Thread);
    }

    /* Reset the quantum */
    Thread->QuantumTarget = CycleTime.QuadPart + KiCyclesPerClockQuantum * Thread->QuantumReset;

    // For Windows 8+ Direct Switch
    if (KiClearThreadQuantumDonationFlag(Thread))
    {
        // todo (andrew.boyarshin)
    }
}

/*
 * @implemented
 */
KPRIORITY
NTAPI
KeSetPriorityThread(IN PKTHREAD Thread,
                    IN KPRIORITY Priority)
{
    KIRQL OldIrql;
    KPRIORITY OldPriority;
    ASSERT_THREAD(Thread);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);
    ASSERT((Priority <= HIGH_PRIORITY) && (Priority >= LOW_PRIORITY));
    ASSERT(KeIsExecutingDpc() == FALSE);

    /* Lock the Dispatcher Database */
    OldIrql = KiAcquireDispatcherLock();

    /* Lock the thread */
    KiAcquireThreadLock(Thread);

    const PKPRCB Prcb = KeGetCurrentPrcb();
    const BOOLEAN CurrentThread = Thread == Prcb->CurrentThread;

    /* Save the old Priority and reset decrement */
    OldPriority = Thread->Priority;
    Thread->PriorityDecrement = 0;

    /* Make sure that an actual change is being done */
    if (Priority != Thread->Priority)
    {
        KiSetQuantumTargetThread(Prcb, Thread, CurrentThread);

        /* Check if priority is being set too low and normalize if so */
        if (Thread->BasePriority && !Priority) Priority = 1;

        /* Set the new Priority */
        KiSetPriorityThread(Thread, Priority);
    }

    /* Release thread lock */
    KiReleaseThreadLock(Thread);

    /* Release the dispatcher database */
    KiReleaseDispatcherLock(OldIrql);

    /* Return Old Priority */
    return OldPriority;
}

/*
 * @implemented
 */
VOID
NTAPI
KeTerminateThread(IN KPRIORITY Increment)
{
    PLIST_ENTRY *ListHead;
    PETHREAD Entry, SavedEntry;
    PETHREAD *ThreadAddr;
    KLOCK_QUEUE_HANDLE LockHandle;
    PKTHREAD Thread = KeGetCurrentThread();
    PKPROCESS Process = Thread->ApcState.Process;
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    if (Thread->CalloutActive)
        KeBugCheckEx(0x107u, (ULONG_PTR)Thread, 0, 0, 0);

    /* Lock the process */
    KiAcquireProcessLockRaiseToSynch(Process, &LockHandle);

    /* Make sure we won't get Swapped */
    Thread->WaitReason = WrTerminated;
    //KiSetThreadSwapBusy(Thread);

    /* Save the Kernel and User Times */
    Process->KernelTime += Thread->KernelTime;
    Process->UserTime += Thread->UserTime;

    /* Get the current entry and our Port */
    Entry = (PETHREAD)PspReaperListHead.Flink;
    ThreadAddr = &((PETHREAD)Thread)->ReaperLink;

    /* Add it to the reaper's list */
    do
    {
        /* Get the list head */
        ListHead = &PspReaperListHead.Flink;

        /* Link ourselves */
        *ThreadAddr = Entry;
        SavedEntry = Entry;

        /* Now try to do the exchange */
        Entry = InterlockedCompareExchangePointer((PVOID*)ListHead,
                                                  ThreadAddr,
                                                  Entry);

        /* Break out if the change was succesful */
    } while (Entry != SavedEntry);

    /* Acquire the dispatcher lock */
    KiAcquireDispatcherLockAtSynchLevel();

    /* Check if the reaper wasn't active */
    if (!Entry)
    {
        /* Activate it as a work item, directly through its Queue */
        KiInsertQueue(&ExWorkerQueue[HyperCriticalWorkQueue].WorkerQueue,
                      &PspReaperWorkItem.List,
                      FALSE);
    }

    /* Check the thread has an associated queue */
    if (Thread->Queue)
    {
        /* Remove it from the list, and handle the queue */
        RemoveEntryList(&Thread->QueueListEntry);
        KiActivateWaiterQueue(Thread->Queue);
    }

    /* Signal the thread */
    Thread->Header.SignalState = TRUE;
    if (!IsListEmpty(&Thread->Header.WaitListHead))
    {
        /* Unwait the threads */
        KxUnwaitThread(&Thread->Header, Increment);
    }

    /* Remove the thread from the list */
    RemoveEntryList(&Thread->ThreadListEntry);

    /* Release the process lock */
    KiReleaseProcessLockFromSynchLevel(&LockHandle);

    /* Set us as terminated, decrease the Process's stack count */
    Thread->State = Terminated;

    /* Decrease stack count */
    ASSERT(Process->StackCount != 0);
    ASSERT(Process->State == ProcessInMemory);
    Process->StackCount--;
    if (!(Process->StackCount) && !(IsListEmpty(&Process->ThreadListHead)))
    {
        /* FIXME: Swap stacks */
    }

    /* Rundown arch-specific parts */
    KiRundownThread(Thread);

    /* Swap to a new thread */
    KiReleaseDispatcherLockFromSynchLevel();
    KiSwapThread(Thread, KeGetCurrentPrcb());
}

THREAD_PRIORITY_VALUES
NTAPI
KiExtractThreadPriorityValues(IN BYTE Priority)
{
    THREAD_PRIORITY_VALUES Result;

    Result.FullPriority = Priority;
    // w / 16 % 16
    Result.Major = ((BYTE)Priority >> 4) & 0xF;
    // w % 16
    Result.Minor = (BYTE)Priority & 0xF;

    return Result;
}

void
NTAPI
KeSetPriorityBoost(IN PKTHREAD Thread, IN KPRIORITY Priority)
{
    KIRQL OldIrql;
    ASSERT_THREAD(Thread);
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);
    ASSERT(Priority <= LOW_REALTIME_PRIORITY);

    if (Thread->Process == &KiInitialProcess.Pcb)
    {
        return;
    }

    /* Lock the Dispatcher Database */
    OldIrql = KiAcquireDispatcherLock();

    /* Lock the thread */
    KiAcquireThreadLock(Thread);

    const PKPRCB Prcb = KeGetCurrentPrcb();
    const BOOLEAN CurrentThread = Thread == Prcb->CurrentThread;

    /* Make sure that an actual change is being done */
    if (Priority > Thread->Priority)
    {
        THREAD_PRIORITY_VALUES Value = KiExtractThreadPriorityValues(Thread->PriorityDecrement);
        ASSERT(Value.Major + Value.Minor <= Thread->Priority);

        const KPRIORITY Diff = Priority - Thread->Priority;
        ASSERT(Diff && Diff < LOW_REALTIME_PRIORITY);

        Thread->PriorityDecrement += Diff * LOW_REALTIME_PRIORITY;

        Value = KiExtractThreadPriorityValues(Thread->PriorityDecrement);
        ASSERT(Value.Major + Value.Minor <= Thread->Priority);

        KiSetQuantumTargetThread(Prcb, Thread, CurrentThread);

        // Check if priority is being set too low
        ASSERT(!Thread->BasePriority || Priority);

        /* Set the new Priority */
        KiSetPriorityThread(Thread, Priority);
    }

    /* Release thread lock */
    KiReleaseThreadLock(Thread);

    /* Release the dispatcher lock */
    KiReleaseDispatcherLock(OldIrql);
}

