/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/iomgr/irq.c
 * PURPOSE:         I/O Wrappers (called Completion Ports) for Kernel Queues
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

#if 0
PKINTERRUPT
NTAPI
KeAllocateInterrupt(IN PKPRCB Prcb)
{
    /* Allocate the array of I/O Interrupts */
    PKINTERRUPT Holder = ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(KINTERRUPT),
        TAG_KINTERRUPT_HOLDER
    );

    if (!Holder)
        return NULL;

    RtlZeroMemory(Holder, sizeof(KINTERRUPT));

    return Holder;
}

void
NTAPI
KeFreeInterrupt(IN PKINTERRUPT Interrupt)
{
    ExFreePoolWithTag(Interrupt, TAG_KINTERRUPT_HOLDER);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IopConnectInterrupt(OUT PIO_INTERRUPT* InterruptObject,
                    IN PKSERVICE_ROUTINE ServiceRoutine,
                    IN PVOID ServiceContext,
                    IN PKSPIN_LOCK SpinLock,
                    IN ULONG Vector,
                    IN KIRQL Irql,
                    IN KIRQL SynchronizeIrql,
                    IN KINTERRUPT_MODE InterruptMode,
                    IN BOOLEAN ShareVector,
                    IN PGROUP_AFFINITY Affinity,
                    IN BOOLEAN FloatingSave)
{
    PIO_INTERRUPT IoInterrupt;
    PKSPIN_LOCK SpinLockUsed;
    NTSTATUS Status;
    GROUP_AFFINITY AffinityCopy = *Affinity;

    PAGED_CODE();
    ASSERT_IRQL_LESS_OR_EQUAL(PASSIVE_LEVEL);

    /* Assume failure */
    *InterruptObject = NULL;

    /* Get the affinity */
    const ULONG Count = KeCountSetBitsGroupAffinity(&AffinityCopy);

    /* Make sure we have a valid CPU count */
    if (!Count || !KeVerifyGroupAffinity(&AffinityCopy, FALSE))
        return STATUS_INVALID_PARAMETER;

    const ULONG_PTR InterruptSize = FIELD_OFFSET(IO_INTERRUPT, Interrupt[Count]);

    /* Allocate the array of I/O Interrupts */
    IoInterrupt = ExAllocatePoolWithTag(
        NonPagedPool,
        InterruptSize,
        TAG_KINTERRUPT
        );

    if (!IoInterrupt)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Select which Spinlock to use */
    SpinLockUsed = SpinLock ? SpinLock : &IoInterrupt->SpinLock;

    /* Start with a fresh structure */
    RtlZeroMemory(IoInterrupt, InterruptSize);

    IoInterrupt->Affinity = AffinityCopy;
    Status = STATUS_SUCCESS;

    /* Now create all the interrupts */
    for (ULONG i = 0; i < Count; i++)
    {
        const PKPRCB Prcb = KiConsumeNextProcessor(&AffinityCopy);

        if (!Prcb)
            break;

        /* Check which one we will use */
        const PKINTERRUPT Interrupt = i == 0 ? &IoInterrupt->InterruptObject : KeAllocateInterrupt(Prcb);

        if (!Interrupt)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        /* Initialize it */
        KeInitializeInterrupt(
            Interrupt,
            ServiceRoutine,
            ServiceContext,
            SpinLockUsed,
            Vector,
            Irql,
            SynchronizeIrql,
            InterruptMode,
            ShareVector,
            Prcb->Number,
            FloatingSave);

        IoInterrupt->Interrupt[i] = Interrupt;

    }

    // AffinityCopy is empty now (consumed by KiConsumeNextProcessor)

    if (NT_SUCCESS(Status))
    {
        /* Connect it */
        Status = KeConnectInterrupt((PKINTERRUPT*)&IoInterrupt->Interrupt, Count);

        if (NT_SUCCESS(Status))
        {
            IoInterrupt->InterruptObject = *IoInterrupt->Interrupt[0];
        }
    }

    if (!NT_SUCCESS(Status))
    {
        for (ULONG j = 0; j < Count; j++)
        {
            const PKINTERRUPT CurrentInterrupt = IoInterrupt->Interrupt[j];

            if (CurrentInterrupt)
                KeFreeInterrupt(CurrentInterrupt);
        }

        ExFreePoolWithTag(IoInterrupt, TAG_KINTERRUPT);

        /* And fail */
        return STATUS_INVALID_PARAMETER;
    }

    /* Return Success */
    return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS
NTAPI
IopConnectInterruptFullySpecified(OUT PKINTERRUPT *InterruptObject,
                                  IN PKSERVICE_ROUTINE ServiceRoutine,
                                  IN PVOID ServiceContext,
                                  IN PKSPIN_LOCK SpinLock,
                                  IN ULONG Vector,
                                  IN KIRQL Irql,
                                  IN KIRQL SynchronizeIrql,
                                  IN KINTERRUPT_MODE InterruptMode,
                                  IN BOOLEAN ShareVector,
                                  IN PGROUP_AFFINITY Affinity,
                                  IN BOOLEAN FloatingSave)
{
    PIO_INTERRUPT IoInterrupt = NULL;

    if (!ServiceRoutine || !KeVerifyGroupAffinity(Affinity, FALSE))
        return STATUS_INVALID_PARAMETER;

    if (SynchronizeIrql && SynchronizeIrql < Irql)
        return STATUS_INVALID_PARAMETER;

    NTSTATUS Status = IopConnectInterrupt(
        &IoInterrupt,
        ServiceRoutine,
        ServiceContext,
        SpinLock,
        Vector,
        Irql,
        SynchronizeIrql,
        InterruptMode,
        ShareVector,
        Affinity,
        FloatingSave
    );

    if (NT_SUCCESS(Status))
        *InterruptObject = &IoInterrupt->InterruptObject;

    return Status;
}


/*
 * @implemented
 */
NTSTATUS
NTAPI
IoConnectInterrupt(OUT PKINTERRUPT *InterruptObject,
                   IN PKSERVICE_ROUTINE ServiceRoutine,
                   IN PVOID ServiceContext,
                   IN PKSPIN_LOCK SpinLock,
                   IN ULONG Vector,
                   IN KIRQL Irql,
                   IN KIRQL SynchronizeIrql,
                   IN KINTERRUPT_MODE InterruptMode,
                   IN BOOLEAN ShareVector,
                   IN KAFFINITY ProcessorEnableMask,
                   IN BOOLEAN FloatingSave)
{
    GROUP_AFFINITY Affinity = { 0 };

    if (KeGetCurrentIrql())
    {
        KeBugCheckEx(DRIVER_VIOLATION, 0, KeGetCurrentIrql(), 0, 0);
    }

    Affinity.Group = 0;
    Affinity.Mask = ProcessorEnableMask & KeActiveProcessors.Bitmap[Affinity.Group];

    return IopConnectInterruptFullySpecified(
        InterruptObject,
        ServiceRoutine,
        ServiceContext,
        SpinLock,
        Vector,
        Irql,
        SynchronizeIrql,
        InterruptMode,
        ShareVector,
        &Affinity,
        FloatingSave
    );
}

/*
 * @implemented
 */
VOID
NTAPI
IoDisconnectInterrupt(PKINTERRUPT InterruptObject)
{
    PIO_INTERRUPT IoInterrupt;
    PAGED_CODE();

    ASSERT_IRQL_LESS_OR_EQUAL(PASSIVE_LEVEL);

    /* Get the I/O Interrupt */
    IoInterrupt = CONTAINING_RECORD(InterruptObject,
                                    IO_INTERRUPT,
                                    InterruptObject);

    const ULONG Count = KeCountSetBitsGroupAffinity(&IoInterrupt->Affinity);

    ASSERT(Count);

    /* Disconnect the interrupt chain */
    KeDisconnectInterrupt((PKINTERRUPT *)&IoInterrupt->Interrupt, Count);

    /* Now free the allocated chain */
    for (ULONG i = 0; i < Count; i++)
    {
        /* Make sure one was registered */
        ASSERT(IoInterrupt->Interrupt[i]);

        KeFreeInterrupt(IoInterrupt->Interrupt[i]);
    }

    /* Free the I/O Interrupt */
    ExFreePool(IoInterrupt); // ExFreePoolWithTag(IoInterrupt, TAG_KINTERRUPT);
}
#else

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoConnectInterrupt(
    OUT PKINTERRUPT *InterruptObject,
    IN PKSERVICE_ROUTINE ServiceRoutine,
    IN PVOID ServiceContext,
    IN PKSPIN_LOCK SpinLock,
    IN ULONG Vector,
    IN KIRQL Irql,
    IN KIRQL SynchronizeIrql,
    IN KINTERRUPT_MODE InterruptMode,
    IN BOOLEAN ShareVector,
    IN KAFFINITY ProcessorEnableMask,
    IN BOOLEAN FloatingSave)
{
    PKINTERRUPT Interrupt;
    PKINTERRUPT InterruptUsed;
    PIO_INTERRUPT IoInterrupt;
    PKSPIN_LOCK SpinLockUsed;
    BOOLEAN FirstRun;
    CCHAR Count = 0;
    KAFFINITY Affinity;
    PAGED_CODE();

    /* Assume failure */
    *InterruptObject = NULL;

    /* Get the affinity */
    Affinity = ProcessorEnableMask & KeActiveProcessors.Bitmap[0];
    while (Affinity)
    {
        /* Increase count */
        if (Affinity & 1)
            Count++;
        Affinity >>= 1;
    }

    /* Make sure we have a valid CPU count */
    if (!Count)
        return STATUS_INVALID_PARAMETER;

    /* Allocate the array of I/O Interrupts */
    IoInterrupt =
        ExAllocatePoolWithTag(NonPagedPool, (Count - 1) * sizeof(KINTERRUPT) + sizeof(IO_INTERRUPT), TAG_KINTERRUPT);
    if (!IoInterrupt)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Select which Spinlock to use */
    SpinLockUsed = SpinLock ? SpinLock : &IoInterrupt->SpinLock;

    /* We first start with a built-in Interrupt inside the I/O Structure */
    *InterruptObject = &IoInterrupt->FirstInterrupt;
    Interrupt = (PKINTERRUPT)(IoInterrupt + 1);
    FirstRun = TRUE;

    /* Start with a fresh structure */
    RtlZeroMemory(IoInterrupt, sizeof(IO_INTERRUPT));

    /* Now create all the interrupts */
    Affinity = ProcessorEnableMask & KeActiveProcessors.Bitmap[0];
    for (Count = 0; Affinity; Count++, Affinity >>= 1)
    {
        /* Check if it's enabled for this CPU */
        if (Affinity & 1)
        {
            /* Check which one we will use */
            InterruptUsed = FirstRun ? &IoInterrupt->FirstInterrupt : Interrupt;

            /* Initialize it */
            KeInitializeInterrupt(
                InterruptUsed, ServiceRoutine, ServiceContext, SpinLockUsed, Vector, Irql, SynchronizeIrql,
                InterruptMode, ShareVector, Count, FloatingSave);

            /* Connect it */
            if (!KeConnectInterrupt(InterruptUsed))
            {
                /* Check how far we got */
                if (FirstRun)
                {
                    /* We failed early so just free this */
                    ExFreePoolWithTag(IoInterrupt, TAG_KINTERRUPT);
                }
                else
                {
                    /* Far enough, so disconnect everything */
                    IoDisconnectInterrupt(&IoInterrupt->FirstInterrupt);
                }

                /* And fail */
                return STATUS_INVALID_PARAMETER;
            }

            /* Now we've used up our First Run */
            if (FirstRun)
            {
                FirstRun = FALSE;
            }
            else
            {
                /* Move on to the next one */
                IoInterrupt->Interrupt[(UCHAR)Count] = Interrupt++;
            }
        }
    }

    /* Return Success */
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
VOID NTAPI
IoDisconnectInterrupt(PKINTERRUPT InterruptObject)
{
    LONG i;
    PIO_INTERRUPT IoInterrupt;
    PAGED_CODE();

    /* Get the I/O Interrupt */
    IoInterrupt = CONTAINING_RECORD(InterruptObject, IO_INTERRUPT, FirstInterrupt);

    /* Disconnect the first one */
    KeDisconnectInterrupt(&IoInterrupt->FirstInterrupt);

    /* Now disconnect the others */
    for (i = 0; i < KeNumberProcessors; i++)
    {
        /* Make sure one was registered */
        if (IoInterrupt->Interrupt[i])
        {
            /* Disconnect it */
            KeDisconnectInterrupt(&InterruptObject[i]);
        }
    }

    /* Free the I/O Interrupt */
    ExFreePool(IoInterrupt); // ExFreePoolWithTag(IoInterrupt, TAG_KINTERRUPT);
}
#endif

/* EOF */
