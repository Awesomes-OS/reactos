#include <ldrp.h>

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrGetDllHandleEx(IN ULONG Flags,
                  IN PWSTR DllPath OPTIONAL,
                  IN PULONG DllCharacteristics OPTIONAL,
                  IN PUNICODE_STRING DllName,
                  OUT PVOID* DllHandle OPTIONAL)
{
    LDR_FUNC(NTSTATUS, Flags, DllPath, DllCharacteristics, DllName, DllHandle)

        NTSTATUS Status;
        PLDR_DATA_TABLE_ENTRY LdrEntry = NULL;
        LDRP_PATH_SEARCH_CONTEXT PathSearchContext = {0};

        LDR_WATCH_LOCALS(Status, LdrEntry, PathSearchContext);

        /* Clear the handle */
        if (DllHandle)
            *DllHandle = NULL;

        const BOOLEAN Pin = Flags & LDR_GET_DLL_HANDLE_EX_PIN;
        const BOOLEAN UnchangedRefCount = Flags & LDR_GET_DLL_HANDLE_EX_UNCHANGED_REFCOUNT;
        Flags &= ~LDR_GET_DLL_HANDLE_EX_PIN;
        Flags &= ~LDR_GET_DLL_HANDLE_EX_UNCHANGED_REFCOUNT;

        LDR_WATCH_LOCALS(Pin, UnchangedRefCount);

        /* Check for a valid flag combination */
        if (Flags)
        {
            LDR_ERROR("Unexpected flags: " LDR_HEX32_FMT, Flags);
            Status = STATUS_INVALID_PARAMETER;
            goto Quickie;
        }

        if (!DllHandle && !Pin)
        {
            LDR_ERROR("No DllHandle given");
            Status = STATUS_INVALID_PARAMETER;
            goto Quickie;
        }

        if (DllPath)
        {
            // ReactOS-specific hack to only search in LdrpHashTable
            // That is, only using LdrpFindLoadedDllByName.
            // Now we ignore that, LdrpSearchPath will use LdrpDefaultPath instead if necessary.
            if (DllPath == (PWSTR)1)
            {
                RtlInitEmptyUnicodeString(&PathSearchContext.DllSearchPath, NULL, 0);
            }
            else
            {
                ASSERT_SUCCESS_OR_RETURN(RtlInitUnicodeStringEx(&PathSearchContext.DllSearchPath, DllPath));
            }
        }
        else
        {
            PathSearchContext.DllSearchPath = LdrpDefaultPath;
        }

        /* Do the lookup */
        Status = LdrpFindLoadedDll(DllName, &PathSearchContext, &LdrEntry);

    Quickie:
        /* The success path must have a valid loader entry */
        ASSERT_EQ(NT_SUCCESS(Status), (LdrEntry != NULL));

        /* Check if we got an entry and success */
        LDR_TRACE("Got LdrEntry");
        if (LdrEntry && NT_SUCCESS(Status))
        {
            if (Pin)
            {
                Status = LdrpPinModule(LdrEntry);
            }
            else if (!UnchangedRefCount)
            {
                Status = LdrpIncrementModuleLoadCount(LdrEntry);
            }

            /* Check if the caller is requesting the handle */
            if (DllHandle && NT_SUCCESS(Status))
                *DllHandle = LdrEntry->DllBase;

            LdrpDereferenceModule(LdrEntry);
        }

        /* Return */
        return Status;
    LDR_FUNC_END_RETURN
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrGetDllHandle(IN PWSTR DllPath OPTIONAL,
                IN PULONG DllCharacteristics OPTIONAL,
                IN PUNICODE_STRING DllName,
                OUT PVOID* DllHandle)
{
    LDR_FUNC(NTSTATUS, DllPath, DllCharacteristics, DllName, DllHandle)
        /* Call the newer API */
        return LdrGetDllHandleEx(LDR_GET_DLL_HANDLE_EX_UNCHANGED_REFCOUNT,
                                 DllPath,
                                 DllCharacteristics,
                                 DllName,
                                 DllHandle);
    LDR_FUNC_END_RETURN
}
