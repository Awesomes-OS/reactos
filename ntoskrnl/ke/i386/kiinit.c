/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/i386/kiinit.c
 * PURPOSE:         Kernel Initialization for x86 CPUs
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>
#include "internal/i386/trap_x.h"

/* GLOBALS *******************************************************************/

/* Boot and double-fault/NMI/DPC stack */
UCHAR DECLSPEC_ALIGN(PAGE_SIZE) P0BootStackData[KERNEL_STACK_SIZE] = {0};
UCHAR DECLSPEC_ALIGN(PAGE_SIZE) KiDoubleFaultStackData[KERNEL_STACK_SIZE] = {0};
ULONG_PTR P0BootStack = (ULONG_PTR)&P0BootStackData[KERNEL_STACK_SIZE];
ULONG_PTR KiDoubleFaultStack = (ULONG_PTR)&KiDoubleFaultStackData[KERNEL_STACK_SIZE];

/* Spinlocks used only on X86 */
KSPIN_LOCK KiFreezeExecutionLock;
KSPIN_LOCK Ki486CompatibilityLock;

/* Perf */
ULONG ProcessCount;
ULONGLONG BootCycles, BootCyclesEnd;

/* FUNCTIONS *****************************************************************/

CODE_SEG("INIT")
VOID
NTAPI
KiInitMachineDependent(VOID)
{
    ULONG CpuCount;
    BOOLEAN FbCaching = FALSE;
    NTSTATUS Status;
    ULONG ReturnLength;
    ULONG i, Affinity, Sample = 0;
    PFX_SAVE_AREA FxSaveArea;
    ULONG MXCsrMask = 0xFFBF;
    CPU_INFO CpuInfo;
    KI_SAMPLE_MAP Samples[10];
    PKI_SAMPLE_MAP CurrentSample = Samples;
    LARGE_IDENTITY_MAP IdentityMap;

    /* Check for large page support */
    if (KeFeatureBits & KF_LARGE_PAGE)
    {
        /* Do an IPI to enable it on all CPUs */
        if (Ki386CreateIdentityMap(&IdentityMap, Ki386EnableCurrentLargePage, 2))
            KeIpiGenericCall(Ki386EnableTargetLargePage, (ULONG_PTR)&IdentityMap);

        /* Free the pages allocated for identity map */
        Ki386FreeIdentityMap(&IdentityMap);
    }

    /* Check for global page support */
    if (KeFeatureBits & KF_GLOBAL_PAGE)
    {
        /* Do an IPI to enable it on all CPUs */
        CpuCount = KeNumberProcessors;
        KeIpiGenericCall(Ki386EnableGlobalPage, (ULONG_PTR)&CpuCount);
    }

    /* Check for PAT and/or MTRR support */
    if (KeFeatureBits & (KF_PAT | KF_MTRR))
    {
        /* Query the HAL to make sure we can use it */
        Status = HalQuerySystemInformation(HalFrameBufferCachingInformation,
                                           sizeof(BOOLEAN),
                                           &FbCaching,
                                           &ReturnLength);
        if ((NT_SUCCESS(Status)) && (FbCaching))
        {
            /* We can't, disable it */
            KeFeatureBits &= ~(KF_PAT | KF_MTRR);
        }
    }

    /* Check for PAT support and enable it */
    if (KeFeatureBits & KF_PAT) KiInitializePAT();

    /* Assume no errata for now */
    SharedUserData->ProcessorFeatures[PF_FLOATING_POINT_PRECISION_ERRATA] = 0;

    /* Check if we have an NPX */
    if (KeI386NpxPresent)
    {
        /* Loop every CPU */
        i = KeActiveProcessors;
        for (Affinity = 1; i; Affinity <<= 1)
        {
            /* Check if this is part of the set */
            if (i & Affinity)
            {
                /* Run on this CPU */
                i &= ~Affinity;
                KeSetSystemAffinityThread(Affinity);

                /* Detect FPU errata */
            if (KiIsNpxErrataPresent())
            {
                /* Disable NPX support */
                KeI386NpxPresent = FALSE;
                SharedUserData->
                    ProcessorFeatures[PF_FLOATING_POINT_PRECISION_ERRATA] =
                    TRUE;
                    break;
                }
            }
        }
    }

    /* If there's no NPX, then we're emulating the FPU */
    SharedUserData->ProcessorFeatures[PF_FLOATING_POINT_EMULATED] =
        !KeI386NpxPresent;

    /* Check if there's no NPX, so that we can disable associated features */
    if (!KeI386NpxPresent)
    {
        /* Remove NPX-related bits */
        KeFeatureBits &= ~(KF_XMMI64 | KF_XMMI | KF_FXSR | KF_MMX);

        /* Disable kernel flags */
        KeI386FxsrPresent = KeI386XMMIPresent = FALSE;

        /* Disable processor features that might've been set until now */
        SharedUserData->ProcessorFeatures[PF_FLOATING_POINT_PRECISION_ERRATA] =
        SharedUserData->ProcessorFeatures[PF_XMMI64_INSTRUCTIONS_AVAILABLE]   =
        SharedUserData->ProcessorFeatures[PF_XMMI_INSTRUCTIONS_AVAILABLE]     =
        SharedUserData->ProcessorFeatures[PF_3DNOW_INSTRUCTIONS_AVAILABLE]    =
        SharedUserData->ProcessorFeatures[PF_MMX_INSTRUCTIONS_AVAILABLE] = 0;
    }

    /* Check for CR4 support */
    if (KeFeatureBits & KF_CR4)
    {
        /* Do an IPI call to enable the Debug Exceptions */
        CpuCount = KeNumberProcessors;
        KeIpiGenericCall(Ki386EnableDE, (ULONG_PTR)&CpuCount);
    }

    /* Check if FXSR was found */
    if (KeFeatureBits & KF_FXSR)
    {
        /* Do an IPI call to enable the FXSR */
        CpuCount = KeNumberProcessors;
        KeIpiGenericCall(Ki386EnableFxsr, (ULONG_PTR)&CpuCount);

        /* Check if XMM was found too */
        if (KeFeatureBits & KF_XMMI)
        {
            /* Do an IPI call to enable XMMI exceptions */
            CpuCount = KeNumberProcessors;
            KeIpiGenericCall(Ki386EnableXMMIExceptions, (ULONG_PTR)&CpuCount);

            /* FIXME: Implement and enable XMM Page Zeroing for Mm */

            /* Patch the RtlPrefetchMemoryNonTemporal routine to enable it */
            *(PCHAR)RtlPrefetchMemoryNonTemporal = 0x90; // NOP
        }
    }

    /* Check for, and enable SYSENTER support */
    KiRestoreFastSyscallReturnState();

    /* Loop every CPU */
    i = KeActiveProcessors;
    for (Affinity = 1; i; Affinity <<= 1)
    {
        /* Check if this is part of the set */
        if (i & Affinity)
        {
            /* Run on this CPU */
            i &= ~Affinity;
            KeSetSystemAffinityThread(Affinity);

            /* Reset MHz to 0 for this CPU */
        KeGetCurrentPrcb()->MHz = 0;

        /* Check if we can use RDTSC */
        if (KeFeatureBits & KF_RDTSC)
        {
            /* Start sampling loop */
            for (;;)
            {
                /* Do a dummy CPUID to start the sample */
                KiCpuId(&CpuInfo, 0);

                /* Fill out the starting data */
                CurrentSample->PerfStart = KeQueryPerformanceCounter(NULL);
                CurrentSample->TSCStart = __rdtsc();
                CurrentSample->PerfFreq.QuadPart = -50000;

                /* Sleep for this sample */
                KeDelayExecutionThread(KernelMode,
                                       FALSE,
                                       &CurrentSample->PerfFreq);

                /* Do another dummy CPUID */
                KiCpuId(&CpuInfo, 0);

                /* Fill out the ending data */
                CurrentSample->PerfEnd =
                    KeQueryPerformanceCounter(&CurrentSample->PerfFreq);
                CurrentSample->TSCEnd = __rdtsc();

                /* Calculate the differences */
                CurrentSample->PerfDelta = CurrentSample->PerfEnd.QuadPart -
                                           CurrentSample->PerfStart.QuadPart;
                CurrentSample->TSCDelta = CurrentSample->TSCEnd -
                                          CurrentSample->TSCStart;

                /* Compute CPU Speed */
                CurrentSample->MHz = (ULONG)((CurrentSample->TSCDelta *
                                              CurrentSample->
                                              PerfFreq.QuadPart + 500000) /
                                             (CurrentSample->PerfDelta *
                                              1000000));

                /* Check if this isn't the first sample */
                if (Sample)
                {
                    /* Check if we got a good precision within 1MHz */
                    if ((CurrentSample->MHz == CurrentSample[-1].MHz) ||
                        (CurrentSample->MHz == CurrentSample[-1].MHz + 1) ||
                        (CurrentSample->MHz == CurrentSample[-1].MHz - 1))
                    {
                        /* We did, stop sampling */
                        break;
                    }
                }

                /* Move on */
                CurrentSample++;
                Sample++;

                if (Sample == RTL_NUMBER_OF(Samples))
                {
                    /* No luck. Average the samples and be done */
                    ULONG TotalMHz = 0;
                    while (Sample--)
                    {
                        TotalMHz += Samples[Sample].MHz;
                    }
                    CurrentSample[-1].MHz = TotalMHz / RTL_NUMBER_OF(Samples);
                    DPRINT1("Sampling CPU frequency failed. Using average of %lu MHz\n", CurrentSample[-1].MHz);
                    break;
                }
            }

            /* Save the CPU Speed */
            KeGetCurrentPrcb()->MHz = CurrentSample[-1].MHz;
        }

        /* Check if we have MTRR */
            if (KeFeatureBits & KF_MTRR)
            {
                /* Then manually initialize MTRR for the CPU */
                KiInitializeMTRR(i ? FALSE : TRUE);
            }

        /* Check if we have AMD MTRR and initialize it for the CPU */
        if (KeFeatureBits & KF_AMDK6MTRR) KiAmdK6InitializeMTRR();

        /* Check if this is a buggy Pentium and apply the fixup if so */
        if (KiI386PentiumLockErrataPresent) KiI386PentiumLockErrataFixup();

        /* Check if the CPU supports FXSR */
        if (KeFeatureBits & KF_FXSR)
        {
            /* Get the current thread NPX state */
            FxSaveArea = KiGetThreadNpxArea(KeGetCurrentThread());

            /* Clear initial MXCsr mask */
            FxSaveArea->U.FxArea.MXCsrMask = 0;

            /* Save the current NPX State */
            Ke386SaveFpuState(FxSaveArea);

            /* Check if the current mask doesn't match the reserved bits */
            if (FxSaveArea->U.FxArea.MXCsrMask != 0)
            {
                /* Then use whatever it's holding */
                MXCsrMask = FxSaveArea->U.FxArea.MXCsrMask;
            }

            /* Check if nobody set the kernel-wide mask */
            if (!KiMXCsrMask)
            {
                /* Then use the one we calculated above */
                KiMXCsrMask = MXCsrMask;
            }
            else
            {
                /* Was it set to the same value we found now? */
                if (KiMXCsrMask != MXCsrMask)
                {
                    /* No, something is definitely wrong */
                    KeBugCheckEx(MULTIPROCESSOR_CONFIGURATION_NOT_SUPPORTED,
                                 KF_FXSR,
                                 KiMXCsrMask,
                                 MXCsrMask,
                                 0);
                }
            }

            /* Now set the kernel mask */
                KiMXCsrMask &= MXCsrMask;
            }
        }
    }

    /* Return affinity back to where it was */
    KeRevertToUserAffinityThread();

    /* NT allows limiting the duration of an ISR with a registry key */
    if (KiTimeLimitIsrMicroseconds)
    {
        /* FIXME: TODO */
        DPRINT1("ISR Time Limit not yet supported\n");
    }

    /* Set CR0 features based on detected CPU */
    KiSetCR0Bits();
}

CODE_SEG("INIT")
VOID
NTAPI
KiInitializePcr(IN ULONG ProcessorNumber,
                IN PKIPCR Pcr,
                IN PKIDTENTRY Idt,
                IN PKGDTENTRY Gdt,
                IN PKTSS Tss,
                IN PKTHREAD IdleThread,
                IN PVOID DpcStack)
{
    /* Setup the TIB */
    Pcr->NtTib.ExceptionList = EXCEPTION_CHAIN_END;
    Pcr->NtTib.StackBase = 0;
    Pcr->NtTib.StackLimit = 0;
    Pcr->NtTib.Self = NULL;

    /* Set the Current Thread */
    Pcr->PrcbData.CurrentThread = IdleThread;

    /* Set pointers to ourselves */
    Pcr->SelfPcr = (PKPCR)Pcr;
    Pcr->Prcb = &Pcr->PrcbData;

    /* Set the PCR Version */
    Pcr->MajorVersion = PCR_MAJOR_VERSION;
    Pcr->MinorVersion = PCR_MINOR_VERSION;

    /* Set the PCRB Version */
    Pcr->PrcbData.MajorVersion = PCR_MAJOR_VERSION;
    Pcr->PrcbData.MinorVersion = PCR_MINOR_VERSION;

    /* Set the Build Type */
    Pcr->PrcbData.BuildType = 0;
#ifndef CONFIG_SMP
    Pcr->PrcbData.BuildType |= PRCB_BUILD_UNIPROCESSOR;
#endif
#if DBG
    Pcr->PrcbData.BuildType |= PRCB_BUILD_DEBUG;
#endif

    /* Set the Processor Number and current Processor Mask */
    Pcr->PrcbData.Number = (UCHAR)ProcessorNumber;
    Pcr->PrcbData.SetMember = 1 << ProcessorNumber;

    /* Set the PRCB for this Processor */
    KiProcessorBlock[ProcessorNumber] = Pcr->Prcb;

    /* Start us out at PASSIVE_LEVEL */
    Pcr->Irql = PASSIVE_LEVEL;

    /* Set the GDI, IDT, TSS and DPC Stack */
    Pcr->GDT = (PVOID)Gdt;
    Pcr->IDT = Idt;
    Pcr->TSS = Tss;
    Pcr->TssCopy = Tss;
    Pcr->PrcbData.DpcStack = DpcStack;

    /* Setup the processor set */
    Pcr->PrcbData.MultiThreadProcessorSet = Pcr->PrcbData.SetMember;
}

CODE_SEG("INIT")
void
NTAPI
KiInitializeNxSupportDiscard(void)
{
    const PSTR Options = KeLoaderBlock->LoadOptions;

    /* Set the default NX policy (opt-in) */
    SharedUserData->NXSupportPolicy = NX_SUPPORT_POLICY_OPTIN;

    if (strstr(Options, "NOEXECUTE"))
    {
        if (!strstr(Options, "NOEXECUTE=OPTIN"))
        {
            if (strstr(Options, "NOEXECUTE=ALWAYSON"))
            {
                SharedUserData->NXSupportPolicy = NX_SUPPORT_POLICY_ALWAYSON;
            }
            else if (strstr(Options, "NOEXECUTE=OPTOUT"))
            {
                SharedUserData->NXSupportPolicy = NX_SUPPORT_POLICY_OPTOUT;
            }
            else if (strstr(Options, "NOEXECUTE=ALWAYSOFF"))
            {
                SharedUserData->NXSupportPolicy = NX_SUPPORT_POLICY_ALWAYSOFF;
            }
        }
    }
    else if (strstr(Options, "EXECUTE"))
    {
        SharedUserData->NXSupportPolicy = NX_SUPPORT_POLICY_ALWAYSOFF;
    }
}

CODE_SEG("INIT")
VOID
NTAPI
KiInitializeKernel(IN PEPROCESS InitProcess,
                   IN PETHREAD InitThread,
                   IN PVOID IdleStack,
                   IN PKPRCB Prcb,
                   IN CCHAR Number,
                   IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    BOOLEAN NpxPresent;
    ULONG FeatureBits;
    ULONG PageDirectory[2];
    PVOID DpcStack;
    ULONG Vendor[3];
    KIRQL DummyIrql;

    /* Detect and set the CPU Type */
    KiSetProcessorType();

    /* Check if an FPU is present */
    NpxPresent = KiIsNpxPresent();

    /* Initialize the Power Management Support for this PRCB */
    PoInitializePrcb(Prcb);

    /* Bugcheck if this is a 386 CPU */
    if (Prcb->CpuType == 3) KeBugCheckEx(UNSUPPORTED_PROCESSOR, 0x386, 0, 0, 0);

    /* Get the processor features for the CPU */
    FeatureBits = KiGetFeatureBits();

    KiInitializeNxSupportDiscard();

    switch (SharedUserData->NXSupportPolicy)
    {
        case NX_SUPPORT_POLICY_ALWAYSON:
        case NX_SUPPORT_POLICY_OPTIN:
        case NX_SUPPORT_POLICY_OPTOUT:
            FeatureBits |= KF_NX_ENABLED;
            break;

        case NX_SUPPORT_POLICY_ALWAYSOFF:
            FeatureBits |= KF_NX_DISABLED;
            break;

        DEFAULT_UNREACHABLE;
    }

    /* Save feature bits */
    Prcb->FeatureBits = FeatureBits;

    /* Save CPU state */
    KiSaveProcessorControlState(&Prcb->ProcessorState);

    /* Get cache line information for this CPU */
    KiGetCacheInformation();

    /* Initialize spinlocks and DPC data */
    KiInitSpinLocks(Prcb, Number);

    /* Check if this is the Boot CPU */
    if (!Number)
    {
        /* Set Node Data */
        KeNodeBlock[0] = &KiNode0;
        Prcb->ParentNode = KeNodeBlock[0];
        KeNodeBlock[0]->ProcessorMask = Prcb->SetMember;

        /* Set boot-level flags */
        KeI386NpxPresent = NpxPresent;
        KeI386CpuType = Prcb->CpuType;
        KeI386CpuStep = Prcb->CpuStep;
        KeProcessorArchitecture = PROCESSOR_ARCHITECTURE_INTEL;
        KeProcessorLevel = (USHORT)Prcb->CpuType;
        if (Prcb->CpuID) KeProcessorRevision = Prcb->CpuStep;
        KeFeatureBits = FeatureBits;
        KeI386FxsrPresent = (KeFeatureBits & KF_FXSR) ? TRUE : FALSE;
        KeI386XMMIPresent = (KeFeatureBits & KF_XMMI) ? TRUE : FALSE;

        /* Detect 8-byte compare exchange support */
        if (!(KeFeatureBits & KF_CMPXCHG8B))
        {
            /* Copy the vendor string */
            RtlCopyMemory(Vendor, Prcb->VendorString, sizeof(Vendor));

            /* Bugcheck the system. Windows *requires* this */
            KeBugCheckEx(UNSUPPORTED_PROCESSOR,
                         (1 << 24 ) | (Prcb->CpuType << 16) | Prcb->CpuStep,
                         Vendor[0],
                         Vendor[1],
                         Vendor[2]);
        }

        /* Set the current MP Master KPRCB to the Boot PRCB */
        Prcb->MultiThreadSetMaster = Prcb;

        /* Lower to APC_LEVEL */
        KeLowerIrql(APC_LEVEL);

        /* Initialize some spinlocks */
        KeInitializeSpinLock(&KiFreezeExecutionLock);
        KeInitializeSpinLock(&Ki486CompatibilityLock);

        /* Initialize portable parts of the OS */
        KiInitSystem();

        /* Initialize the Idle Process and the Process Listhead */
        InitializeListHead(&KiProcessListHead);
        PageDirectory[0] = 0;
        PageDirectory[1] = 0;
        KeInitializeProcess(&InitProcess->Pcb,
                            0,
                            0xFFFFFFFF,
                            PageDirectory,
                            FALSE);
        InitProcess->Pcb.QuantumReset = MAXCHAR;
    }
    else
    {
        /* FIXME */
        DPRINT1("SMP Boot support not yet present\n");
    }

    /* Setup the Idle Thread */
    if (NT_SUCCESS(KeInitThread(&InitThread->Tcb, IdleStack, NULL, NULL, NULL, NULL, NULL, &InitProcess->Pcb)))
    {
        /* Start it */
        KeStartThread(&InitThread->Tcb);
    }

    InitThread->Tcb.NextProcessor = Number;
    InitThread->Tcb.Priority = HIGH_PRIORITY;
    InitThread->Tcb.State = Running;
    InitThread->Tcb.Affinity = 1 << Number;
    InitThread->Tcb.WaitIrql = DISPATCH_LEVEL;
    InitProcess->Pcb.ActiveProcessors = 1 << Number;

    /* HACK for MmUpdatePageDir */
    ((PETHREAD)InitThread)->ThreadsProcess = (PEPROCESS)InitProcess;

    /* Set basic CPU Features that user mode can read */
    SharedUserData->ProcessorFeatures[PF_MMX_INSTRUCTIONS_AVAILABLE] =
        (KeFeatureBits & KF_MMX) ? TRUE: FALSE;
    SharedUserData->ProcessorFeatures[PF_COMPARE_EXCHANGE_DOUBLE] =
        (KeFeatureBits & KF_CMPXCHG8B) ? TRUE: FALSE;
    SharedUserData->ProcessorFeatures[PF_XMMI_INSTRUCTIONS_AVAILABLE] =
        ((KeFeatureBits & KF_FXSR) && (KeFeatureBits & KF_XMMI)) ? TRUE: FALSE;
    SharedUserData->ProcessorFeatures[PF_XMMI64_INSTRUCTIONS_AVAILABLE] =
        ((KeFeatureBits & KF_FXSR) && (KeFeatureBits & KF_XMMI64)) ? TRUE: FALSE;
    SharedUserData->ProcessorFeatures[PF_3DNOW_INSTRUCTIONS_AVAILABLE] =
        (KeFeatureBits & KF_3DNOW) ? TRUE: FALSE;
    SharedUserData->ProcessorFeatures[PF_RDTSC_INSTRUCTION_AVAILABLE] =
        (KeFeatureBits & KF_RDTSC) ? TRUE: FALSE;

    /* Set up the thread-related fields in the PRCB */
    Prcb->CurrentThread = &InitThread->Tcb;
    Prcb->NextThread = NULL;
    Prcb->IdleThread = &InitThread->Tcb;

    /* Initialize the Kernel Executive */
    ExpInitializeExecutive(Number, LoaderBlock);

    /* Only do this on the boot CPU */
    if (!Number)
    {
        /* Calculate the time reciprocal */
        KiTimeIncrementReciprocal =
            KiComputeReciprocal(KeMaximumIncrement,
                                &KiTimeIncrementShiftCount);

        /* Update DPC Values in case they got updated by the executive */
        Prcb->MaximumDpcQueueDepth = KiMaximumDpcQueueDepth;
        Prcb->MinimumDpcRate = KiMinimumDpcRate;
        Prcb->AdjustDpcThreshold = KiAdjustDpcThreshold;

        KiVBoxPrint("KiInitializeKernel 11\n");

        /* Allocate the DPC Stack */
        DpcStack = MmCreateKernelStack(FALSE, 0);
        if (!DpcStack) KeBugCheckEx(NO_PAGES_AVAILABLE, 1, 0, 0, 0);
        Prcb->DpcStack = DpcStack;

        /* Allocate the IOPM save area */
        Ki386IopmSaveArea = ExAllocatePoolWithTag(PagedPool,
                                                  IOPM_SIZE,
                                                  '  eK');
        if (!Ki386IopmSaveArea)
        {
            /* Bugcheck. We need this for V86/VDM support. */
            KeBugCheckEx(NO_PAGES_AVAILABLE, 2, IOPM_SIZE, 0, 0);
        }
    }

    /* Raise to Dispatch */
    KeRaiseIrql(DISPATCH_LEVEL, &DummyIrql);

    /* Set the Idle Priority to 0. This will jump into Phase 1 */
    KeSetPriorityThread(&InitThread->Tcb, 0);

    /* If there's no thread scheduled, put this CPU in the Idle summary */
    KiAcquirePrcbLock(Prcb);
    if (!Prcb->NextThread) KiIdleSummary |= 1 << Number;
    KiReleasePrcbLock(Prcb);

    /* Raise back to HIGH_LEVEL and clear the PRCB for the loader block */
    KeRaiseIrql(HIGH_LEVEL, &DummyIrql);
    LoaderBlock->Prcb = 0;
}

CODE_SEG("INIT")
VOID
FASTCALL
KiGetMachineBootPointers(IN PKGDTENTRY *Gdt,
                         IN PKIDTENTRY *Idt,
                         IN PKIPCR *Pcr,
                         IN PKTSS *Tss)
{
    KDESCRIPTOR GdtDescriptor, IdtDescriptor;
    KGDTENTRY TssSelector, PcrSelector;
    USHORT Tr, Fs;

    /* Get GDT and IDT descriptors */
    Ke386GetGlobalDescriptorTable(&GdtDescriptor.Limit);
    __sidt(&IdtDescriptor.Limit);

    /* Save IDT and GDT */
    *Gdt = (PKGDTENTRY)GdtDescriptor.Base;
    *Idt = (PKIDTENTRY)IdtDescriptor.Base;

    /* Get TSS and FS Selectors */
    Tr = Ke386GetTr();
    Fs = Ke386GetFs();

    /* Get PCR Selector, mask it and get its GDT Entry */
    PcrSelector = *(PKGDTENTRY)((ULONG_PTR)*Gdt + (Fs & ~RPL_MASK));

    /* Get the KPCR itself */
    *Pcr = (PKIPCR)(ULONG_PTR)(PcrSelector.BaseLow |
                               PcrSelector.HighWord.Bytes.BaseMid << 16 |
                               PcrSelector.HighWord.Bytes.BaseHi << 24);

    /* Get TSS Selector, mask it and get its GDT Entry */
    TssSelector = *(PKGDTENTRY)((ULONG_PTR)*Gdt + (Tr & ~RPL_MASK));

    /* Get the KTSS itself */
    *Tss = (PKTSS)(ULONG_PTR)(TssSelector.BaseLow |
                              TssSelector.HighWord.Bytes.BaseMid << 16 |
                              TssSelector.HighWord.Bytes.BaseHi << 24);
}

BOOLEAN
NTAPI
KeAreInterruptsEnabled(VOID)
{
    const unsigned int flags = __getcallerseflags();
    // 9 - Interrupt Flag
    return (flags >> 9) & 1;
};

CODE_SEG("INIT")
VOID
NTAPI
KiSystemStartupBootStack(VOID)
{
    const PEPROCESS Process = &KiInitialProcess; //(PEPROCESS)KeLoaderBlock->Process;
    PETHREAD Thread = (PETHREAD)KeLoaderBlock->Thread;

    /* Initialize the kernel for the current CPU */
    KiInitializeKernel(Process,
                       Thread,
                       (PVOID)(KeLoaderBlock->KernelStack & ~3),
                       (PKPRCB)__readfsdword(KPCR_PRCB),
                       KeNumberProcessors - 1,
                       KeLoaderBlock);

    /* Set the priority of this thread to 0 */
    KeGetCurrentThread()->Priority = 0;

    /* Force interrupts enabled and lower IRQL back to DISPATCH_LEVEL */
#if 0
    ASSERT(KeAreInterruptsEnabled());
#else
    _enable();
#endif
    KeLowerIrql(DISPATCH_LEVEL);

    /* Set the right wait IRQL */
    KeGetCurrentThread()->WaitIrql = DISPATCH_LEVEL;

    KiVBoxPrint("KiSystemStartupBootStack before IDLE loop\n");

    /* Jump into the idle loop */
    KiIdleLoop();
}

static
VOID
KiMarkPageAsReadOnly(
    PVOID Address)
{
    PHARDWARE_PTE PointerPte;

    /* Make sure the address is page aligned */
    ASSERT(ALIGN_DOWN_POINTER_BY(Address, PAGE_SIZE) == Address);

    /* Get the PTE address */
    PointerPte = ((PHARDWARE_PTE)PTE_BASE) + ((ULONG_PTR)Address / PAGE_SIZE);
    ASSERT(PointerPte->Valid);
    ASSERT(PointerPte->Write);

    /* Set as read-only */
    PointerPte->Write = 0;

    /* Flush the TLB entry */
    __invlpg(Address);
}

void
NTAPI
KiVBoxPrint(const char *s)
{
    UCHAR c;

    while ((c = *s))
    {
        WRITE_PORT_UCHAR((PUCHAR)0x504, c);
        s++;
    }
}

void
NTAPI
KiVBoxPrintInteger(ULONG Value)
{
    CHAR Buffer[33];

    if (NT_SUCCESS(RtlIntegerToChar(Value, 10, sizeof(Buffer), Buffer)))
    {
        KiVBoxPrint(Buffer);
    }
    else
    {
        KiVBoxPrint("<fail>");
    }
}

CODE_SEG("INIT")
VOID
NTAPI
KiSystemStartup(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    ULONG Cpu;
    PKTHREAD InitialThread;
#if 0
    PKPROCESS InitialProcess;
#endif
    ULONG InitialStack;
    PKGDTENTRY Gdt;
    PKIDTENTRY Idt;
    KIDTENTRY NmiEntry, DoubleFaultEntry;
    PKTSS Tss;
    PKIPCR Pcr;
    KIRQL DummyIrql;

    /* Boot cycles timestamp */
    BootCycles = __rdtsc();

    KiVBoxPrint("KiSystemStartup 1\n");

    /* Save the loader block and get the current CPU */
    KeLoaderBlock = LoaderBlock;
    Cpu = KeNumberProcessors;
    if (!Cpu)
    {
        /* If this is the boot CPU, set FS and the CPU Number*/
        Ke386SetFs(KGDT_R0_PCR);
        __writefsbyte(KPCR_PRCB_DATA + FIELD_OFFSET(KPRCB, Number), Cpu);

        /* Set the initial stack and idle thread as well */
        LoaderBlock->KernelStack = (ULONG_PTR)P0BootStack;
        LoaderBlock->Thread = (ULONG_PTR)&KiInitialThread;
#if 0
        LoaderBlock->Process = (ULONG_PTR)&KiInitialProcess;
#endif
    }

    /* Save the initial thread and stack */
    InitialStack = LoaderBlock->KernelStack;
    InitialThread = (PKTHREAD)LoaderBlock->Thread;
#if 0
    InitialProcess = (PKPROCESS)LoaderBlock->Process;
#endif

    /* Clean the APC List Head */
    InitializeListHead(&InitialThread->ApcState.ApcListHead[KernelMode]);

    /* Initialize the machine type */
    KiInitializeMachineType();

    /* Skip initial setup if this isn't the Boot CPU */
    if (Cpu) goto AppCpuInit;

    /* Get GDT, IDT, PCR and TSS pointers */
    KiGetMachineBootPointers(&Gdt, &Idt, &Pcr, &Tss);

    /* Setup the TSS descriptors and entries */
    Ki386InitializeTss(Tss, Idt, Gdt);

    KiVBoxPrint("KiSystemStartup 1.4\n");

    /* Initialize the PCR */
    RtlZeroMemory(Pcr, (sizeof(*Pcr) + PAGE_SIZE - 1UL) & (~(PAGE_SIZE-1UL)));
    KiInitializePcr(Cpu,
                    Pcr,
                    Idt,
                    Gdt,
                    Tss,
                    InitialThread, (PVOID)KiDoubleFaultStack);

    KiVBoxPrint("KiSystemStartup 1.5\n");

    /* Set us as the current process */
#if 0
    InitialThread->ApcState.Process = InitialProcess;
#else
    InitialThread->ApcState.Process = &KiInitialProcess.Pcb;
#endif

    /* Clear DR6/7 to cleanup bootloader debugging */
    //KeGetPcr()->NtTib.Self = NULL;
    __writefsdword(KPCR_TEB, 0);
    KeGetCurrentPrcb()->ProcessorState.SpecialRegisters.KernelDr6 = 0;
    KeGetCurrentPrcb()->ProcessorState.SpecialRegisters.KernelDr7 = 0;

    KiVBoxPrint("KiSystemStartup 1.6\n");

    /* Setup the IDT */
    KeInitExceptions();

    /* Load Ring 3 selectors for DS/ES */
    Ke386SetDs(KGDT_R3_DATA | RPL_MASK);
    Ke386SetEs(KGDT_R3_DATA | RPL_MASK);

    KiVBoxPrint("KiSystemStartup 1.7\n");

    /* Save NMI and double fault traps */
    RtlCopyMemory(&NmiEntry, &Idt[2], sizeof(KIDTENTRY));
    RtlCopyMemory(&DoubleFaultEntry, &Idt[8], sizeof(KIDTENTRY));

    /* Copy kernel's trap handlers */
    RtlCopyMemory(Idt,
                  (PVOID)KiIdtDescriptor.Base,
                  KiIdtDescriptor.Limit + 1);

    /* Restore NMI and double fault */
    RtlCopyMemory(&Idt[2], &NmiEntry, sizeof(KIDTENTRY));
    RtlCopyMemory(&Idt[8], &DoubleFaultEntry, sizeof(KIDTENTRY));

AppCpuInit:
    /* Loop until we can release the freeze lock */
    do
    {
        /* Loop until execution can continue */
        while (*(volatile PKSPIN_LOCK*)&KiFreezeExecutionLock == (PVOID)1);
    } while(InterlockedBitTestAndSet((PLONG)&KiFreezeExecutionLock, 0));

    /* Setup CPU-related fields */
    __writefsdword(KPCR_NUMBER, Cpu);
    __writefsdword(KPCR_SET_MEMBER, 1 << Cpu);
    __writefsdword(KPCR_SET_MEMBER_COPY, 1 << Cpu);
    __writefsdword(KPCR_PRCB_DATA + FIELD_OFFSET(KPRCB, SetMember), 1 << Cpu);
    //KeGetCurrentPrcb()->SetMember = 1 << Cpu;

    /* Initialize the Processor with HAL */
    HalInitializeProcessor(Cpu, KeLoaderBlock);

    /* Set active processors */
    KeActiveProcessors |= __readfsdword(KPCR_SET_MEMBER);
    KeNumberProcessors++;

    /* Check if this is the boot CPU */
    if (!Cpu)
    {
        /* Initialize debugging system */
        KdInitSystem(0, KeLoaderBlock);

        /* Check for break-in */
        if (KdPollBreakIn()) DbgBreakPointWithStatus(DBG_STATUS_CONTROL_C);

        /* Make the lowest page of the boot and double fault stack read-only */
        KiMarkPageAsReadOnly(P0BootStackData);
        KiMarkPageAsReadOnly(KiDoubleFaultStackData);
    }

    /* Raise to HIGH_LEVEL */
    KeRaiseIrql(HIGH_LEVEL, &DummyIrql);

    KiVBoxPrint("KiSystemStartup 6\n");

    /* Switch to new kernel stack and start kernel bootstrapping */
    KiSwitchToBootStack(InitialStack & ~3);
}
