#include <ldrp.h>

/*
 * @implemented
 */
NTSTATUS
NTAPI
LdrGetProcedureAddressForCaller(IN PVOID BaseAddress,
                                IN PANSI_STRING FunctionNameInput OPTIONAL,
                                IN ULONG Ordinal OPTIONAL,
                                OUT PVOID* ProcedureAddress,
                                IN UINT8 Flags,
                                IN PVOID* CallbackAddress)
{
    const BOOLEAN WorkWithNotInitializedModules = Flags & 1;
    const LDR_DDAG_STATE MinimumRequiredState = WorkWithNotInitializedModules
                                                    ? LdrModulesCondensed
                                                    : LdrModulesReadyToRun;

    PANSI_STRING FunctionName = NULL;
    BOOLEAN AllocatedFunctionName = FALSE;

    if (FunctionNameInput)
    {
        if (FunctionNameInput->Length + 1 > FunctionNameInput->MaximumLength
            || FunctionNameInput->Buffer[FunctionNameInput->Length])
        {
            FunctionName = (PANSI_STRING)LdrpHeapAlloc(0, sizeof(ANSI_STRING));
            if (!FunctionName)
                return STATUS_INSUFFICIENT_RESOURCES;

            FunctionName->Buffer = (PCHAR)LdrpHeapAlloc(0, FunctionNameInput->Length + 1);
            if (!FunctionName->Buffer)
            {
                LdrpHeapFree(0, FunctionName);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            FunctionName->Length = FunctionNameInput->Length;
            FunctionName->MaximumLength = FunctionNameInput->Length + 1;
            RtlMoveMemory(FunctionName->Buffer, FunctionNameInput->Buffer, FunctionName->Length);
            FunctionName->Buffer[FunctionName->Length] = '\0';
            AllocatedFunctionName = TRUE;
        }
        else
        {
            FunctionName = FunctionNameInput;
        }
    }

    PLDR_DATA_TABLE_ENTRY LdrEntry;
    LDR_DDAG_STATE LdrEntryState;
    NTSTATUS Status;
    LDRP_RESOLVE_PROCEDURE_ADDRESS_FLAGS ResolveProcedureAddressFlags = {0};
    ResolveProcedureAddressFlags.DoSecurityVerification = TRUE;

    while (TRUE)
    {
        LdrEntry = NULL;
        LdrEntryState = LdrModulesPlaceHolder;

        Status = LdrpFindLoadedDllByHandle(BaseAddress, &LdrEntry, &LdrEntryState);

        if (!NT_SUCCESS(Status))
        {
            Status = STATUS_DLL_NOT_FOUND;
            break;
        }

        if (NtCurrentTeb()->LoadOwner || LdrEntryState >= MinimumRequiredState)
        {
            // Ok, found one, it is in appropriate state (or we are load owner), use it.
            break;
        }

        // Something is wrong, we don't want this module
        LdrpDereferenceModule(LdrEntry);

        if (LdrEntryState < LdrModulesPlaceHolder)
        {
            // Loaded with errors
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        // Still not loaded
        LdrpDrainWorkQueue(LoadComplete);
        LdrpDropLastInProgressCount();
    }

    if (LdrEntry && NT_SUCCESS(Status))
    {
        Status = LdrpResolveProcedureAddress(LdrEntry,
                                             FunctionName ? FunctionName->Buffer : NULL,
                                             Ordinal,
                                             ResolveProcedureAddressFlags,
                                             ProcedureAddress);

        if (NT_SUCCESS(Status))
        {
            if (LdrEntryState == LdrModulesReadyToInit && !WorkWithNotInitializedModules)
            {
                if (NtCurrentTeb()->LoadOwner)
                {
                    BOOLEAN HasInitializingNode = FALSE;
                    Status = LdrpInitializeGraphRecurse(LdrEntry->DdagNode, NULL, &HasInitializingNode);
                }
            }

            if (!NT_SUCCESS(Status))
            {
                *ProcedureAddress = NULL;
            }

            // todo: shims
        }

        // Modify result status if forwarder is missing or its initialization has failed
        if (Status == STATUS_DLL_NOT_FOUND || Status == STATUS_DLL_INIT_FAILED)
            Status = STATUS_PROCEDURE_NOT_FOUND;

        LdrpDereferenceModule(LdrEntry);
    }

    if (Status == STATUS_PROCEDURE_NOT_FOUND)
    {
        Status = Ordinal ? STATUS_ORDINAL_NOT_FOUND : STATUS_ENTRYPOINT_NOT_FOUND;
    }

    if (!NT_SUCCESS(Status))
    {
        LdrpReportSnapError(BaseAddress,
                            LdrEntry ? &LdrEntry->BaseDllName : NULL,
                            FunctionName,
                            Ordinal,
                            Status,
                            FALSE);
    }

    if (AllocatedFunctionName)
    {
        LdrpHeapFree(0, FunctionName->Buffer);
        LdrpHeapFree(0, FunctionName);
    }

    return Status;
}
