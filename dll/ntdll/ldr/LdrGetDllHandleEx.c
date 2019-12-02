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
    NTSTATUS Status;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    LDRP_PATH_SEARCH_CONTEXT PathSearchContext = {0};

    /* Initialize state */
    LdrEntry = NULL;

    /* Clear the handle */
    if (DllHandle)
        *DllHandle = NULL;

    const BOOLEAN Pin = Flags & LDR_GET_DLL_HANDLE_EX_PIN;
    const BOOLEAN UnchangedRefCount = Flags & LDR_GET_DLL_HANDLE_EX_UNCHANGED_REFCOUNT;
    Flags &= ~LDR_GET_DLL_HANDLE_EX_PIN;
    Flags &= ~LDR_GET_DLL_HANDLE_EX_UNCHANGED_REFCOUNT;

    /* Check for a valid flag combination */
    if (Flags)
    {
        DPRINT1("Unexpected %s flags: %#X\n", __FUNCTION__, Flags);
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }

    if (!DllHandle && !Pin)
    {
        DPRINT1("No DllHandle given to %s\n", __FUNCTION__);
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }

    /* Display debug string */
    if (ShowSnaps)
    {
        DPRINT1("LDR: %s, searching for %wZ\n",
                __FUNCTION__,
                DllName);
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
            ASSERT(NT_SUCCESS(RtlInitUnicodeStringEx(&PathSearchContext.DllSearchPath, DllPath)));
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
    ASSERT((LdrEntry != NULL) == NT_SUCCESS(Status));

    /* Check if we got an entry and success */
    DPRINT("Got LdrEntry->BaseDllName \"%wZ\"\n", LdrEntry ? &LdrEntry->BaseDllName : NULL);
    if ((LdrEntry) && (NT_SUCCESS(Status)))
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
    /* Call the newer API */
    return LdrGetDllHandleEx(LDR_GET_DLL_HANDLE_EX_UNCHANGED_REFCOUNT,
                             DllPath,
                             DllCharacteristics,
                             DllName,
                             DllHandle);
}
