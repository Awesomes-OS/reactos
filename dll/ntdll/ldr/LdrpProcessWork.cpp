#include <ldrp.h>
#include <ndk/exfuncs.h>

void
NTAPI
LdrpProcessWork(PLDRP_LOAD_CONTEXT LoadContext, LDRP_PROCESS_WORK_MODE Mode)
{
    LDRP_ASSERT_LOAD_CONTEXT(LoadContext);
    ASSERT(Mode == QueuedWork || Mode == StandaloneRequest);

    NTSTATUS Status = *LoadContext->LoadStatus;

    if (NT_SUCCESS(Status))
    {
        LDRP_ASSERT_MODULE_ENTRY(LoadContext->Module);
        LDRP_ASSERT_DDAG_NODE(LoadContext->Module->DdagNode);

        if (LoadContext->Module->DdagNode->State != LdrModulesPlaceHolder)
        {
            ASSERT(LoadContext->Module->DdagNode->State == LdrModulesSnapping);
            Status = LdrpSnapModule(LoadContext);
        }
        else
        {
            if (LoadContext->Flags.InRetryQueue)
            {
                Status = LdrpMapDllRetry(LoadContext);
            }
            else if (LoadContext->Flags.HasFullPath)
            {
                Status = LdrpMapDllFullPath(LoadContext);
            }
            else
            {
                Status = LdrpMapDllSearchPath(LoadContext);
            }

            if (!NT_SUCCESS(Status) && Status != STATUS_RETRY)
            {
                /* Failure */
                DPRINT1(
                    "LDR: Unable to load DLL: \"%wZ\", Parent Module: \"%wZ\", Status: 0x%08lX\n",
                    &LoadContext->DllName, LoadContext->ParentModule ? &LoadContext->ParentModule->FullDllName : NULL,
                    Status);

                if (LdrpDebugFlags.BreakInDebugger)
                    __debugbreak();

                if (Status == STATUS_DLL_NOT_FOUND)
                {
                    /* We couldn't map, is this a static load? */
                    if (LoadContext->Module->ProcessStaticImport)
                    {
                        /*
                         * This is BAD! Static loads are CRITICAL. Bugcheck!
                         * Initialize the strings for the error
                         */

                        ULONG_PTR HardErrorParameters[1];
                        UNICODE_STRING HardErrorDllName;
                        ULONG Response;

                        RtlInitUnicodeString(&HardErrorDllName, LoadContext->DllName.Buffer);

                        /* Set them as error parameters */
                        HardErrorParameters[0] = (ULONG_PTR)&HardErrorDllName;

                        /* Raise the hard error */
                        NtRaiseHardError(STATUS_DLL_NOT_FOUND, 1, 0x00000001, HardErrorParameters, OptionOk, &Response);

                        /* We're back, where we initializing? */
                        if (LdrInitState != 3)
                            LdrpFatalHardErrorCount++;
                    }
                }
            }
        }

        if (!NT_SUCCESS(Status))
            *LoadContext->LoadStatus = Status;
    }
    else
    {
        if (LdrpDebugFlags.BreakInDebugger)
            __debugbreak();
    }

    if (Mode == QueuedWork)
    {
        DPRINT1("LDR: LdrpProcessWork(\"%wZ\", QueuedWork) -> 0x%08lX, LdrpWorkInProgress = %lu\n",
                &LoadContext->DllName, Status, LdrpWorkInProgress);

        RtlEnterCriticalSection(&LdrpWorkQueueLock);

        ASSERT(LdrpWorkInProgress > 1);
        const ULONG NewWorkInProgressValue = --LdrpWorkInProgress;
        BOOLEAN WorkCompleted = IsListEmpty(&LdrpWorkQueue) && NewWorkInProgressValue == 1;

        RtlLeaveCriticalSection(&LdrpWorkQueueLock);

        if (WorkCompleted)
            NtSetEvent(LdrpWorkCompleteEvent, NULL);
    }
}
