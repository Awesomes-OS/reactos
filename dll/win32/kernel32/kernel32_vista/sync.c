#include "k32_vista.h"

#include <ndk/exfuncs.h>

#include "../include/base_x.h"

FORCEINLINE
PLARGE_INTEGER
GetNtTimeout(PLARGE_INTEGER Time, DWORD Timeout)
{
    if (Timeout == INFINITE) return NULL;
    Time->QuadPart = (ULONGLONG)Timeout * -10000;
    return Time;
}

BOOL
WINAPI
SleepConditionVariableCS(PCONDITION_VARIABLE ConditionVariable, PCRITICAL_SECTION CriticalSection, DWORD Timeout)
{
    NTSTATUS Status;
    LARGE_INTEGER Time;

    Status = RtlSleepConditionVariableCS(ConditionVariable, (PRTL_CRITICAL_SECTION)CriticalSection, GetNtTimeout(&Time, Timeout));
    if (!NT_SUCCESS(Status) || Status == STATUS_TIMEOUT)
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }
    return TRUE;
}

BOOL
WINAPI
SleepConditionVariableSRW(PCONDITION_VARIABLE ConditionVariable, PSRWLOCK Lock, DWORD Timeout, ULONG Flags)
{
    NTSTATUS Status;
    LARGE_INTEGER Time;

    Status = RtlSleepConditionVariableSRW(ConditionVariable, Lock, GetNtTimeout(&Time, Timeout), Flags);
    if (!NT_SUCCESS(Status) || Status == STATUS_TIMEOUT)
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }
    return TRUE;
}

/*
* @implemented
*/
BOOL WINAPI InitializeCriticalSectionEx(OUT LPCRITICAL_SECTION lpCriticalSection,
                                        IN DWORD dwSpinCount,
                                        IN DWORD flags)
{
    NTSTATUS Status;

    /* Initialize the critical section */
    Status = RtlInitializeCriticalSectionEx(
        (PRTL_CRITICAL_SECTION)lpCriticalSection,
        dwSpinCount,
        flags);
    if (!NT_SUCCESS(Status))
    {
        /* Set failure code */
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* Success */
    return TRUE;
}


/***********************************************************************
 *           CreateMutexExA   (KERNEL32.@)
 * 
 * @implemented
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateMutexExA( SECURITY_ATTRIBUTES *sa, LPCSTR name, DWORD flags, DWORD access )
{
    ConvertAnsiToUnicodePrologue
    if (!name) return CreateMutexExW(sa, NULL, flags, access);
    ConvertAnsiToUnicodeBody(name)
    if (NT_SUCCESS(Status)) return CreateMutexExW(sa, UnicodeCache->Buffer, flags, access);
    ConvertAnsiToUnicodeEpilogue
}


/***********************************************************************
 *           CreateMutexExW   (KERNEL32.@)
 * 
 * @implemented
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateMutexExW( SECURITY_ATTRIBUTES *sa, LPCWSTR name, DWORD flags, DWORD access )
{
    CreateNtObjectFromWin32ApiPrologue
    CreateNtObjectFromWin32ApiBody(Mutant, sa, name, access, (flags & CREATE_MUTEX_INITIAL_OWNER) != 0);
    CreateNtObjectFromWin32ApiEpilogue
}
