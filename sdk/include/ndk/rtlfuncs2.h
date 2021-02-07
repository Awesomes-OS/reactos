/*++ NDK Version: 0098

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    rtlfuncs2.h

Abstract:

    Function definitions for the Run-Time Library, Part 2

Author:

    Andrew Boyarshin

--*/

#ifndef _RTLFUNCS2_H
#define _RTLFUNCS2_H

//
// Dependencies
//
#include <umtypes.h>

#ifdef __cplusplus
extern "C" {
#endif

//
// String macros
//

_At_(AnsiString->Buffer, _Post_equal_to_(Buffer))
_At_(AnsiString->Length, _Post_equal_to_(0))
_At_(AnsiString->MaximumLength, _Post_equal_to_(BufferSize))
FORCEINLINE
void
RtlInitEmptyAnsiString(
    _Out_ PANSI_STRING AnsiString,
    _Pre_maybenull_ _Pre_readable_size_(BufferSize) __drv_aliasesMem PCHAR Buffer,
    _In_ USHORT BufferSize
)
{
    AnsiString->Length = 0;
    AnsiString->MaximumLength = BufferSize;
    AnsiString->Buffer = Buffer;
}

_At_(UnicodeString->Buffer, _Post_equal_to_(Buffer))
_At_(UnicodeString->Length, _Post_equal_to_(0))
_At_(UnicodeString->MaximumLength, _Post_equal_to_(BufferSize))
FORCEINLINE
void
RtlInitEmptyUnicodeString(
    _Out_ PUNICODE_STRING UnicodeString,
    _Writable_bytes_(BufferSize)
    _When_(BufferSize != 0, _Notnull_)
    __drv_aliasesMem PWSTR Buffer,
    _In_ USHORT BufferSize
)
{
    UnicodeString->Length = 0;
    UnicodeString->MaximumLength = BufferSize;
    UnicodeString->Buffer = Buffer;
}

//
// Debug Functions
//

// protect from function-like DbgPrint macros (browseui:shellutils)
ULONG (__cdecl DbgPrint)(
    _In_z_ _Printf_format_string_ PCSTR Format,
    ...
);

NTSYSAPI
ULONG
__cdecl
DbgPrintEx(
    _In_ ULONG ComponentId,
    _In_ ULONG Level,
    _In_z_ _Printf_format_string_ PCSTR Format,
    ...
);

NTSYSAPI
ULONG
NTAPI
DbgPrompt(
    _In_z_ PCCH Prompt,
    _Out_writes_bytes_(MaximumResponseLength) PCH Response,
    _In_ ULONG MaximumResponseLength
);

#undef DbgBreakPoint
VOID
NTAPI
DbgBreakPoint(
    VOID
);

#ifdef NTOS_MODE_USER

__analysis_noreturn
NTSYSAPI
VOID
NTAPI
RtlAssert(
    _In_ PVOID FailedAssertion,
    _In_ PVOID FileName,
    _In_ ULONG LineNumber,
    _In_opt_z_ PCHAR Message
);

NTSYSAPI
PVOID
NTAPI
RtlEncodePointer(
    _In_ PVOID Pointer
);

NTSYSAPI
PVOID
NTAPI
RtlDecodePointer(
    _In_ PVOID Pointer
);

NTSYSAPI
PVOID
NTAPI
RtlEncodeSystemPointer(
    _In_ PVOID Pointer
);

NTSYSAPI
PVOID
NTAPI
RtlDecodeSystemPointer(
    _In_ PVOID Pointer
);

NTSYSAPI
NTSTATUS
NTAPI
RtlGetLastNtStatus(
    VOID
);

NTSYSAPI
ULONG
NTAPI
RtlGetLastWin32Error(
    VOID
);

NTSYSAPI
VOID
NTAPI
RtlSetLastWin32Error(
    _In_ ULONG LastError
);

NTSYSAPI
VOID
NTAPI
RtlSetLastWin32ErrorAndNtStatusFromNtStatus(
    _In_ NTSTATUS Status
);

NTSYSAPI
NTSTATUS
NTAPI
RtlSetThreadErrorMode(
    _In_ ULONG NewMode,
    _Out_opt_ PULONG OldMode
);

NTSYSAPI
ULONG
NTAPI
RtlGetThreadErrorMode(
    VOID
);

DECLSPEC_NORETURN
NTSYSAPI
VOID
NTAPI
RtlExitUserProcess(
    _In_ NTSTATUS ExitStatus
);

NTSYSAPI
LONG
NTAPI
RtlUnhandledExceptionFilter(
    _In_ struct _EXCEPTION_POINTERS* ExceptionInfo
);

NTSYSAPI
LONG
NTAPI
RtlUnhandledExceptionFilter2(
    _In_ struct _EXCEPTION_POINTERS* ExceptionInfo,
    _In_ ULONG Flags
);

#endif /* NTOS_MODE_USER */


//
// Critical Section/Resource Functions
//
NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteCriticalSection (
    _In_ PRTL_CRITICAL_SECTION CriticalSection
);

NTSYSAPI
NTSTATUS
NTAPI
RtlEnterCriticalSection(
    _In_ PRTL_CRITICAL_SECTION CriticalSection
);

NTSYSAPI
NTSTATUS
NTAPI
RtlInitializeCriticalSection(
    _In_ PRTL_CRITICAL_SECTION CriticalSection
);

NTSYSAPI
NTSTATUS
NTAPI
RtlInitializeCriticalSectionAndSpinCount(
    _In_ PRTL_CRITICAL_SECTION CriticalSection,
    _In_ ULONG SpinCount
);

#define RTL_CRITICAL_SECTION_FLAG_NO_DEBUG_INFO    0x01000000
#define RTL_CRITICAL_SECTION_FLAG_DYNAMIC_SPIN     0x02000000
#define RTL_CRITICAL_SECTION_FLAG_STATIC_INIT      0x04000000
#define RTL_CRITICAL_SECTION_FLAG_RESOURCE_TYPE    0x08000000
#define RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO 0x10000000
#define RTL_CRITICAL_SECTION_ALL_FLAG_BITS         0xFF000000
#define RTL_CRITICAL_SECTION_FLAG_RESERVED         (RTL_CRITICAL_SECTION_ALL_FLAG_BITS & (~(RTL_CRITICAL_SECTION_FLAG_NO_DEBUG_INFO | RTL_CRITICAL_SECTION_FLAG_DYNAMIC_SPIN | RTL_CRITICAL_SECTION_FLAG_STATIC_INIT | RTL_CRITICAL_SECTION_FLAG_RESOURCE_TYPE | RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO)))

NTSYSAPI
NTSTATUS
NTAPI
RtlInitializeCriticalSectionEx(
    _In_ PRTL_CRITICAL_SECTION CriticalSection,
    _In_ ULONG SpinCount,
    _In_ ULONG Flags /* RTL_CRITICAL_SECTION_FLAG_* */
);

NTSYSAPI
ULONG
NTAPI
RtlIsCriticalSectionLocked(
    _In_ PRTL_CRITICAL_SECTION CriticalSection
);

NTSYSAPI
ULONG
NTAPI
RtlIsCriticalSectionLockedByThread(
    _In_ PRTL_CRITICAL_SECTION CriticalSection
);

NTSYSAPI
NTSTATUS
NTAPI
RtlLeaveCriticalSection(
    _In_ PRTL_CRITICAL_SECTION CriticalSection
);

NTSYSAPI
BOOLEAN
NTAPI
RtlTryEnterCriticalSection(
    _In_ PRTL_CRITICAL_SECTION CriticalSection
);

NTSYSAPI
VOID
NTAPI
RtlpUnWaitCriticalSection(
    _In_ PRTL_CRITICAL_SECTION CriticalSection
);

NTSYSAPI
NTSTATUS
NTAPI
RtlpWaitForCriticalSection(
    _In_ PRTL_CRITICAL_SECTION CriticalSection
);

_IRQL_requires_max_(APC_LEVEL)
_When_(Status < 0, _Out_range_(>, 0))
_When_(Status >= 0, _Out_range_(==, 0))
NTSYSAPI
ULONG
NTAPI
RtlNtStatusToDosError(
    _In_ NTSTATUS Status
);

_When_(Status < 0, _Out_range_(>, 0))
_When_(Status >= 0, _Out_range_(==, 0))
NTSYSAPI
ULONG
NTAPI
RtlNtStatusToDosErrorNoTeb(
    _In_ NTSTATUS Status
);

NTSYSAPI
NTSTATUS
NTAPI
RtlMapSecurityErrorToNtStatus(
    _In_ ULONG SecurityError
);

_IRQL_requires_max_(APC_LEVEL)
_When_(Status < 0, _Out_range_(>, 0))
_When_(Status >= 0, _Out_range_(==, 0))
NTSYSAPI
ULONG
NTAPI
RtlNtStatusToDosError(
    _In_ NTSTATUS Status
);

_When_(Status < 0, _Out_range_(>, 0))
_When_(Status >= 0, _Out_range_(==, 0))
NTSYSAPI
ULONG
NTAPI
RtlNtStatusToDosErrorNoTeb(
    _In_ NTSTATUS Status
);

#ifdef NTOS_MODE_USER

NTSYSAPI
VOID
NTAPI
RtlInitializeConditionVariable(OUT PRTL_CONDITION_VARIABLE ConditionVariable);

NTSYSAPI
VOID
NTAPI
RtlWakeConditionVariable(IN OUT PRTL_CONDITION_VARIABLE ConditionVariable);

NTSYSAPI
VOID
NTAPI
RtlWakeAllConditionVariable(IN OUT PRTL_CONDITION_VARIABLE ConditionVariable);

NTSYSAPI
NTSTATUS
NTAPI
RtlSleepConditionVariableCS(IN OUT PRTL_CONDITION_VARIABLE ConditionVariable,
    IN OUT PRTL_CRITICAL_SECTION CriticalSection,
    IN PLARGE_INTEGER TimeOut OPTIONAL);

NTSYSAPI
NTSTATUS
NTAPI
RtlSleepConditionVariableSRW(IN OUT PRTL_CONDITION_VARIABLE ConditionVariable,
    IN OUT PRTL_SRWLOCK SRWLock,
    IN PLARGE_INTEGER TimeOut OPTIONAL,
    IN ULONG Flags);

NTSYSAPI
VOID
NTAPI
RtlInitializeSRWLock(OUT PRTL_SRWLOCK SRWLock);

NTSYSAPI
VOID
NTAPI
RtlAcquireSRWLockShared(IN OUT PRTL_SRWLOCK SRWLock);

NTSYSAPI
VOID
NTAPI
RtlReleaseSRWLockShared(IN OUT PRTL_SRWLOCK SRWLock);

NTSYSAPI
VOID
NTAPI
RtlAcquireSRWLockExclusive(IN OUT PRTL_SRWLOCK SRWLock);

NTSYSAPI
VOID
NTAPI
RtlReleaseSRWLockExclusive(IN OUT PRTL_SRWLOCK SRWLock);

NTSYSAPI
BOOL
NTAPI
RtlIsProcessorFeaturePresent(IN DWORD ProcessorFeature);

BOOLEAN
NTAPI
RtlIsAnyDebuggerPresent(void);

#endif

typedef unsigned char RTLP_FLS_DATA_CLEANUP_MODE;
#define RTLP_FLS_DATA_CLEANUP_RUN_CALLBACKS (1 << 0)
#define RTLP_FLS_DATA_CLEANUP_DEALLOCATE (1 << 1)

NTSYSAPI
void
NTAPI
RtlpFlsInitialize(void);

NTSYSAPI
void
NTAPI
RtlProcessFlsData(IN PVOID FlsData, IN RTLP_FLS_DATA_CLEANUP_MODE Mode);

typedef VOID(NTAPI* PFLS_CALLBACK_FUNCTION) (_In_ PVOID pFlsData);

NTSYSAPI
NTSTATUS
NTAPI
RtlFlsAlloc(PFLS_CALLBACK_FUNCTION pCallback, ULONG32* pFlsIndex);

NTSYSAPI
NTSTATUS
NTAPI
RtlFlsFree(ULONG32 FlsIndex);

NTSYSAPI
NTSTATUS
NTAPI
RtlFlsGetValue(ULONG32 FlsIndex, PVOID* pFlsValue);

NTSYSAPI
NTSTATUS
NTAPI
RtlFlsSetValue(ULONG32 FlsIndex, PVOID FlsValue);

#ifdef __cplusplus
}
#endif

#endif
