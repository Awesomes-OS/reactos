#include "k32_vista.h"

#include <ndk/exfuncs.h>

#include "../kernel32/include/kernel32.h"
#include "../kernel32/include/base_x.h"

VOID
WINAPI
AcquireSRWLockExclusive(PSRWLOCK Lock)
{
    RtlAcquireSRWLockExclusive((PRTL_SRWLOCK)Lock);
}

VOID
WINAPI
AcquireSRWLockShared(PSRWLOCK Lock)
{
    RtlAcquireSRWLockShared((PRTL_SRWLOCK)Lock);
}

VOID
WINAPI
InitializeConditionVariable(PCONDITION_VARIABLE ConditionVariable)
{
    RtlInitializeConditionVariable((PRTL_CONDITION_VARIABLE)ConditionVariable);
}

VOID
WINAPI
InitializeSRWLock(PSRWLOCK Lock)
{
    RtlInitializeSRWLock((PRTL_SRWLOCK)Lock);
}

VOID
WINAPI
ReleaseSRWLockExclusive(PSRWLOCK Lock)
{
    RtlReleaseSRWLockExclusive((PRTL_SRWLOCK)Lock);
}

VOID
WINAPI
ReleaseSRWLockShared(PSRWLOCK Lock)
{
    RtlReleaseSRWLockShared((PRTL_SRWLOCK)Lock);
}

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

VOID
WINAPI
WakeAllConditionVariable(PCONDITION_VARIABLE ConditionVariable)
{
    RtlWakeAllConditionVariable((PRTL_CONDITION_VARIABLE)ConditionVariable);
}

VOID
WINAPI
WakeConditionVariable(PCONDITION_VARIABLE ConditionVariable)
{
    RtlWakeConditionVariable((PRTL_CONDITION_VARIABLE)ConditionVariable);
}


/*
* @implemented
*/
BOOL WINAPI InitializeCriticalSectionEx(OUT LPCRITICAL_SECTION lpCriticalSection,
                                        IN DWORD dwSpinCount,
                                        IN DWORD flags)
{
    NTSTATUS Status;

    /* FIXME: Flags ignored */

    /* Initialize the critical section */
    Status = RtlInitializeCriticalSectionAndSpinCount(
        (PRTL_CRITICAL_SECTION)lpCriticalSection,
        dwSpinCount);
    if (!NT_SUCCESS(Status))
    {
        /* Set failure code */
        SetLastError(RtlNtStatusToDosError(Status));
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


/***********************************************************************
 *           CreateSemaphoreExA   (KERNEL32.@)
 * 
 * @implemented
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateSemaphoreExA( SECURITY_ATTRIBUTES *sa, LONG initial, LONG max,
                                                    LPCSTR name, DWORD flags, DWORD access )
{
    ConvertAnsiToUnicodePrologue
    if (!name) return CreateSemaphoreExW(sa, initial, max, NULL, flags, access);
    ConvertAnsiToUnicodeBody(name)
    if (NT_SUCCESS(Status)) return CreateSemaphoreExW(sa, initial, max, UnicodeCache->Buffer, flags, access);
    ConvertAnsiToUnicodeEpilogue
}


/***********************************************************************
 *           CreateSemaphoreExW   (KERNEL32.@)
 * 
 * @implemented
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateSemaphoreExW( SECURITY_ATTRIBUTES *sa, LONG initial, LONG max,
                                                    LPCWSTR name, DWORD flags, DWORD access )
{
    CreateNtObjectFromWin32ApiPrologue
    CreateNtObjectFromWin32ApiBody(Semaphore, sa, name, access, initial, max);
    CreateNtObjectFromWin32ApiEpilogue
}
