#include <ldrp.h>
#include <inttypes.h>

LONG LdrpLoaderLockAcquisitionCount;

FORCEINLINE
ULONG_PTR
LdrpMakeCookie(VOID)
{
    LDR_FUNC_MANUAL(ULONG_PTR)
        /* Generate a cookie */
        return (((ULONG_PTR)NtCurrentTeb()->RealClientId.UniqueThread & 0xFFF) << 16) |
            (_InterlockedIncrement(&LdrpLoaderLockAcquisitionCount) & 0xFFFF);
    LDR_FUNC_END_IMPL(return 0);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrUnlockLoaderLock(IN ULONG Flags,
                    IN ULONG_PTR Cookie OPTIONAL)
{
    LDR_FUNC(NTSTATUS, Flags, Cookie)
        NTSTATUS Status = STATUS_SUCCESS;

        /* Check for valid flags */
        if (Flags & ~LDR_UNLOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS)
        {
            /* Flags are invalid, check how to fail */
            if (Flags & LDR_UNLOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS)
            {
                /* The caller wants us to raise status */
                RtlRaiseStatus(STATUS_INVALID_PARAMETER_1);
            }

            /* A normal failure */
            return STATUS_INVALID_PARAMETER_1;
        }

        /* If we don't have a cookie, just return */
        if (!Cookie) return STATUS_SUCCESS;

        /* Validate the cookie */
        if ((Cookie & 0xF0000000) ||
            ((Cookie >> 16) ^ (HandleToUlong(NtCurrentTeb()->RealClientId.UniqueThread) & 0xFFF)))
        {
            LDR_ERROR("LdrUnlockLoaderLock() called with an invalid cookie!");

            /* Invalid cookie, check how to fail */
            if (Flags & LDR_UNLOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS)
            {
                /* The caller wants us to raise status */
                RtlRaiseStatus(STATUS_INVALID_PARAMETER_2);
            }

            /* A normal failure */
            return STATUS_INVALID_PARAMETER_2;
        }

        /* Ready to release the lock */
        if (Flags & LDR_UNLOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS)
        {
            /* Do a direct leave */
            RtlLeaveCriticalSection(&LdrpLoaderLock);
        }
        else
        {
            /* Wrap this in SEH, since we're not supposed to raise */
            [&Status]() {
                __try
                {
                    /* Leave the lock */
                    RtlLeaveCriticalSection(&LdrpLoaderLock);
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    /* We should use the LDR Filter instead */
                    Status = GetExceptionCode();
                }
            }();
        }

        /* All done */
        return Status;
    LDR_FUNC_END_RETURN
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrLockLoaderLock(IN ULONG Flags,
                  OUT PULONG Disposition OPTIONAL,
                  OUT PULONG_PTR Cookie OPTIONAL)
{
    LDR_FUNC(NTSTATUS, Flags, Disposition, Cookie)
        NTSTATUS Status = STATUS_SUCCESS;

        /* Zero out the outputs */
        if (Disposition) *Disposition = LDR_LOCK_LOADER_LOCK_DISPOSITION_INVALID;
        if (Cookie) *Cookie = 0;

        /* Validate the flags */
        if (Flags & ~(LDR_LOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS |
            LDR_LOCK_LOADER_LOCK_FLAG_TRY_ONLY))
        {
            /* Flags are invalid, check how to fail */
            if (Flags & LDR_LOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS)
            {
                /* The caller wants us to raise status */
                RtlRaiseStatus(STATUS_INVALID_PARAMETER_1);
            }

            /* A normal failure */
            return STATUS_INVALID_PARAMETER_1;
        }

        /* Make sure we got a cookie */
        if (!Cookie)
        {
            /* No cookie check how to fail */
            if (Flags & LDR_LOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS)
            {
                /* The caller wants us to raise status */
                RtlRaiseStatus(STATUS_INVALID_PARAMETER_3);
            }

            /* A normal failure */
            return STATUS_INVALID_PARAMETER_3;
        }

        /* If the flag is set, make sure we have a valid pointer to use */
        if ((Flags & LDR_LOCK_LOADER_LOCK_FLAG_TRY_ONLY) && !(Disposition))
        {
            /* No pointer to return the data to */
            if (Flags & LDR_LOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS)
            {
                /* The caller wants us to raise status */
                RtlRaiseStatus(STATUS_INVALID_PARAMETER_2);
            }

            /* Fail */
            return STATUS_INVALID_PARAMETER_2;
        }

        /* Return now if we are in the init phase */
        if (LdrInitState != 3) return STATUS_SUCCESS;

        /* Check what locking semantic to use */
        if (Flags & LDR_LOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS)
        {
            /* Check if we should enter or simply try */
            if (Flags & LDR_LOCK_LOADER_LOCK_FLAG_TRY_ONLY)
            {
                /* Do a try */
                if (!RtlTryEnterCriticalSection(&LdrpLoaderLock))
                {
                    /* It's locked */
                    *Disposition = LDR_LOCK_LOADER_LOCK_DISPOSITION_LOCK_NOT_ACQUIRED;
                }
                else
                {
                    /* It worked */
                    *Disposition = LDR_LOCK_LOADER_LOCK_DISPOSITION_LOCK_ACQUIRED;
                    *Cookie = LdrpMakeCookie();
                }
            }
            else
            {
                /* Do a enter */
                RtlEnterCriticalSection(&LdrpLoaderLock);

                /* See if result was requested */
                if (Disposition) *Disposition = LDR_LOCK_LOADER_LOCK_DISPOSITION_LOCK_ACQUIRED;
                *Cookie = LdrpMakeCookie();
            }
        }
        else
        {
            /* Wrap this in SEH, since we're not supposed to raise */
            [&]() {
                __try
                {
                    /* Check if we should enter or simply try */
                    if (Flags & LDR_LOCK_LOADER_LOCK_FLAG_TRY_ONLY)
                    {
                        /* Do a try */
                        if (!RtlTryEnterCriticalSection(&LdrpLoaderLock))
                        {
                            /* It's locked */
                            *Disposition = LDR_LOCK_LOADER_LOCK_DISPOSITION_LOCK_NOT_ACQUIRED;
                        }
                        else
                        {
                            /* It worked */
                            *Disposition = LDR_LOCK_LOADER_LOCK_DISPOSITION_LOCK_ACQUIRED;
                            *Cookie = LdrpMakeCookie();
                        }
                    }
                    else
                    {
                        /* Do an enter */
                        RtlEnterCriticalSection(&LdrpLoaderLock);

                        /* See if result was requested */
                        if (Disposition) *Disposition = LDR_LOCK_LOADER_LOCK_DISPOSITION_LOCK_ACQUIRED;
                        *Cookie = LdrpMakeCookie();
                    }
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    /* We should use the LDR Filter instead */
                    Status = GetExceptionCode();
                }
            }();
        }

        /* Return status */
        return Status;
    LDR_FUNC_END_RETURN
}


VOID
NTAPI
LdrpEnsureLoaderLockIsHeld(VOID)
{
    LDR_FUNC_VOID_MANUAL()
        // Ignored atm
    LDR_FUNC_END_VOID
}
