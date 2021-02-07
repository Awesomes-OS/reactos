#include <k32.h>
#include <ndk/rtltypes.h>

#define NDEBUG
#include <debug.h>

C_ASSERT(RTL_FLS_MAXIMUM_AVAILABLE == FLS_MAXIMUM_AVAILABLE);

/* PRIVATE FUNCTIONS **********************************************************/

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
DWORD
WINAPI
FlsAlloc(PFLS_CALLBACK_FUNCTION lpCallback)
{
    DWORD FlsIndex = FLS_OUT_OF_INDEXES;
    NTSTATUS Status = RtlFlsAlloc(lpCallback, &FlsIndex);

    if (NT_SUCCESS(Status))
    {
        return FlsIndex;
    }

    BaseSetLastNTError(Status);

    return FLS_OUT_OF_INDEXES;
}


/*
 * @implemented
 */
BOOL
WINAPI
FlsFree(DWORD dwFlsIndex)
{
    NTSTATUS Status = RtlFlsFree(dwFlsIndex);

    if (NT_SUCCESS(Status))
    {
        return TRUE;
    }

    BaseSetLastNTError(Status);

    return FALSE;
}


/*
 * @implemented
 */
PVOID
WINAPI
FlsGetValue(DWORD dwFlsIndex)
{
    PVOID FlsValue = NULL;
    NTSTATUS Status = RtlFlsGetValue(dwFlsIndex, &FlsValue);

    if (NT_SUCCESS(Status))
    {
        SetLastError(ERROR_SUCCESS);
        return FlsValue;
    }

    BaseSetLastNTError(Status);

    return NULL;
}


/*
 * @implemented
 */
BOOL
WINAPI
FlsSetValue(DWORD dwFlsIndex, PVOID lpFlsData)
{
    NTSTATUS Status = RtlFlsSetValue(dwFlsIndex, lpFlsData);

    if (NT_SUCCESS(Status))
    {
        return TRUE;
    }

    BaseSetLastNTError(Status);

    return FALSE;
}
