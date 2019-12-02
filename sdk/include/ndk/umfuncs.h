/*++ NDK Version: 0098

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    umfuncs.h

Abstract:

    Function definitions for Native DLL (ntdll) APIs exclusive to User Mode.

Author:

    Alex Ionescu (alexi@tinykrnl.org) - Updated - 27-Feb-2006

--*/

#ifndef _UMFUNCS_H
#define _UMFUNCS_H

//
// Dependencies
//
#include <umtypes.h>
#include <dbgktypes.h>

//
// Debug Functions
//
__analysis_noreturn
NTSYSAPI
VOID
NTAPI
DbgBreakPointWithStatus(
    _In_ ULONG Status
);

NTSTATUS
NTAPI
DbgUiConnectToDbg(
    VOID
);

NTSTATUS
NTAPI
DbgUiContinue(
    _In_ PCLIENT_ID ClientId,
    _In_ NTSTATUS ContinueStatus
);

NTSTATUS
NTAPI
DbgUiDebugActiveProcess(
    _In_ HANDLE Process
);

NTSTATUS
NTAPI
DbgUiStopDebugging(
    _In_ HANDLE Process
);

NTSYSAPI
NTSTATUS
NTAPI
DbgUiWaitStateChange(
    _In_ PDBGUI_WAIT_STATE_CHANGE DbgUiWaitStateCange,
    _In_ PLARGE_INTEGER TimeOut
);

NTSTATUS
NTAPI
DbgUiConvertStateChangeStructure(
    _In_ PDBGUI_WAIT_STATE_CHANGE WaitStateChange,
    _In_ PVOID DebugEvent
);

VOID
NTAPI
DbgUiRemoteBreakin(
    VOID
);

NTSTATUS
NTAPI
DbgUiIssueRemoteBreakin(
    _In_ HANDLE Process
);

HANDLE
NTAPI
DbgUiGetThreadDebugObject(
    VOID
);

#endif
