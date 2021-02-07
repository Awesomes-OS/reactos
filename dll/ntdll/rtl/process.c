/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS NT User-Mode DLL
 * FILE:            lib/ntdll/rtl/process.c
 * PURPOSE:         RTL Process Lifecycle Routines
 * PROGRAMMERS:     Andrew Boyarshin (andrew.boyarshin@gmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ntdll.h>

#define NDEBUG
#include <debug.h>

#include <subsys/csr/csr.h>
#include <subsys/win/basemsg.h>

VOID
NTAPI
RtlExitUserProcess(
    _In_ NTSTATUS ExitStatus
)
{
    BASE_API_MESSAGE ApiMessage;
    PBASE_EXIT_PROCESS ExitProcessRequest = &ApiMessage.Data.ExitProcessRequest;

    _SEH2_TRY
    {
        /* Acquire the PEB lock */
        RtlAcquirePebLock();

        /* Kill all the threads */
        NtTerminateProcess(NULL, ExitStatus);

        /* Unload all DLLs */
        LdrShutdownProcess();

        /* Notify Base Server of process termination */
        ExitProcessRequest->uExitCode = ExitStatus;
        CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                            NULL,
                            CSR_CREATE_API_NUMBER(BASESRV_SERVERDLL_INDEX, BasepExitProcess),
                            sizeof(*ExitProcessRequest));

        /* Now do it again */
        NtTerminateProcess(NtCurrentProcess(), ExitStatus);
    }
    _SEH2_FINALLY
    {
        /* Release the PEB lock */
        RtlReleasePebLock();
    }
    _SEH2_END;

    /* should never get here */
    ASSERT(0);
    while (1);
}
