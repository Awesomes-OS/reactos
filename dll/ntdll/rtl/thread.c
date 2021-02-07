/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS NT User-Mode DLL
 * FILE:            lib/ntdll/rtl/thread.c
 * PURPOSE:         RTL Thread Lifecycle Routines
 * PROGRAMMERS:     Andrew Boyarshin (andrew.boyarshin@gmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ntdll.h>

#define NDEBUG
#include <debug.h>

#include <subsys/csr/csr.h>
#include <i386/ketypes.h>

/*
 * @implemented
 */
VOID
NTAPI
RtlExitUserThread(NTSTATUS ExitStatus)
{
    NTSTATUS Status;
    ULONG LastThread;

    /* Make sure loader lock isn't held */
    const PRTL_CRITICAL_SECTION LoaderLock = NtCurrentPeb()->LoaderLock;
    if (LoaderLock) ASSERT(NtCurrentTeb()->ClientId.UniqueThread != LoaderLock->OwningThread);

    /*
     * Terminate process if this is the last thread
     * of the current process
     */
    Status = NtQueryInformationThread(NtCurrentThread(),
        ThreadAmILastThread,
        &LastThread,
        sizeof(LastThread),
        NULL);
    if (NT_SUCCESS(Status) && LastThread) RtlExitUserProcess(ExitStatus);

    /* Call the Loader and tell him to notify the DLLs and TLS Callbacks of the ongoing termination */
    LdrShutdownThread();

    /* Shut us down */
    NtCurrentTeb()->FreeStackOnTermination = TRUE;
    NtTerminateThread(NtCurrentThread(), ExitStatus);

    /* We should never reach this place */
    ERROR_FATAL("It should not happen\n");
    while (TRUE); /* 'noreturn' function */
}

#ifdef _M_IX86
typedef unsigned int READETYPE;
#else
typedef unsigned __int64 READETYPE;
#endif

void
NTAPI
RtlGetCurrentProcessorNumberEx(PPROCESSOR_NUMBER ProcNumber)
{
    // todo: rdpid & rdtscp (https://stackoverflow.com/a/22369725)
    const unsigned long seglimit = __segmentlimit(KGDT_DF_TSS + RPL_MASK);
    const READETYPE eflags = __readeflags();
    if (eflags & EFLAGS_ZF)
    {
        ProcNumber->Group = seglimit & 0x3FF;
        ProcNumber->Number = seglimit >> 12 >> 2;
    }
    else
    {
        RtlZeroMemory(ProcNumber, sizeof(*ProcNumber));
        NtGetCurrentProcessorNumberEx(ProcNumber);
    }
}
