/*++ NDK Version: 0098

Copyright (c) ReactOS Contributors.  All rights reserved.

Header Name:

    tpfuncs.h

Abstract:

    Function definitions for the Vista+ Thread Pool Library

--*/

#ifndef _TPFUNCS_H
#define _TPFUNCS_H

#include <winnt.h>
#include <ntdef.h>

NTSYSAPI NTSTATUS  NTAPI TpAllocCleanupGroup(TP_CLEANUP_GROUP **);
NTSYSAPI NTSTATUS  NTAPI TpAllocPool(TP_POOL **,PVOID);
NTSYSAPI NTSTATUS  NTAPI TpAllocTimer(TP_TIMER **,PTP_TIMER_CALLBACK,PVOID,TP_CALLBACK_ENVIRON *);
NTSYSAPI NTSTATUS  NTAPI TpAllocWait(TP_WAIT **,PTP_WAIT_CALLBACK,PVOID,TP_CALLBACK_ENVIRON *);
NTSYSAPI NTSTATUS  NTAPI TpAllocWork(TP_WORK **,PTP_WORK_CALLBACK,PVOID,TP_CALLBACK_ENVIRON *);
NTSYSAPI void      NTAPI TpCallbackLeaveCriticalSectionOnCompletion(TP_CALLBACK_INSTANCE *,RTL_CRITICAL_SECTION *);
NTSYSAPI NTSTATUS  NTAPI TpCallbackMayRunLong(TP_CALLBACK_INSTANCE *);
NTSYSAPI void      NTAPI TpCallbackReleaseMutexOnCompletion(TP_CALLBACK_INSTANCE *,HANDLE);
NTSYSAPI void      NTAPI TpCallbackReleaseSemaphoreOnCompletion(TP_CALLBACK_INSTANCE *,HANDLE,DWORD);
NTSYSAPI void      NTAPI TpCallbackSetEventOnCompletion(TP_CALLBACK_INSTANCE *,HANDLE);
NTSYSAPI void      NTAPI TpCallbackUnloadDllOnCompletion(TP_CALLBACK_INSTANCE *,HMODULE);
NTSYSAPI void      NTAPI TpDisassociateCallback(TP_CALLBACK_INSTANCE *);
NTSYSAPI BOOL      NTAPI TpIsTimerSet(TP_TIMER *);
NTSYSAPI void      NTAPI TpPostWork(TP_WORK *);
NTSYSAPI void      NTAPI TpReleaseCleanupGroup(TP_CLEANUP_GROUP *);
NTSYSAPI void      NTAPI TpReleaseCleanupGroupMembers(TP_CLEANUP_GROUP *,BOOL,PVOID);
NTSYSAPI void      NTAPI TpReleasePool(TP_POOL *);
NTSYSAPI void      NTAPI TpReleaseTimer(TP_TIMER *);
NTSYSAPI void      NTAPI TpReleaseWait(TP_WAIT *);
NTSYSAPI void      NTAPI TpReleaseWork(TP_WORK *);
NTSYSAPI void      NTAPI TpSetPoolMaxThreads(TP_POOL *,DWORD);
NTSYSAPI BOOL      NTAPI TpSetPoolMinThreads(TP_POOL *,DWORD);
NTSYSAPI void      NTAPI TpSetTimer(TP_TIMER *, LARGE_INTEGER *,LONG,LONG);
NTSYSAPI void      NTAPI TpSetWait(TP_WAIT *,HANDLE,LARGE_INTEGER *);
NTSYSAPI NTSTATUS  NTAPI TpSimpleTryPost(PTP_SIMPLE_CALLBACK,PVOID,TP_CALLBACK_ENVIRON *);
NTSYSAPI void      NTAPI TpWaitForTimer(TP_TIMER *,BOOL);
NTSYSAPI void      NTAPI TpWaitForWait(TP_WAIT *,BOOL);
NTSYSAPI void      NTAPI TpWaitForWork(TP_WORK *,BOOL);

#endif