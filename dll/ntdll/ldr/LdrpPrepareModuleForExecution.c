#include <ldrp.h>

NTSTATUS
NTAPI
LdrpPrepareModuleForExecution(IN PLDR_DATA_TABLE_ENTRY LdrEntry,
                              IN NTSTATUS* StatusResponse)
{
    NTSTATUS Status = STATUS_SUCCESS;
    switch (LdrEntry->DdagNode->State)
    {
        case LdrModulesSnapped:
            LdrpCondenseGraph(LdrEntry->DdagNode);
            // fallthrough
        case LdrModulesCondensed:
            if (!LdrEntry->ProcessStaticImport && NtCurrentTeb()->SubProcessTag)
            {
                DPRINT1("LDR: %s(%p[%wZ:%d], ...): tagging not implemented\n",
                        __FUNCTION__,
                        LdrEntry, &LdrEntry->BaseDllName, LdrEntry->DdagNode->State);
                // todo: LdrpAddNodeServiceTag
            }

            Status = LdrpNotifyLoadOfGraph(LdrEntry->DdagNode);
            if (NT_SUCCESS(Status))
            {
                // todo: LdrpDynamicShimModule
            }
            break;

        case LdrModulesReadyToInit:
            break;

        case LdrModulesInitializing:
        case LdrModulesReadyToRun:
            return STATUS_SUCCESS;

        default:
            DPRINT1("%s(%p[%wZ:%d], %p): unexpected state, aborting.\n",
                    __FUNCTION__,
                    LdrEntry, &LdrEntry->BaseDllName, LdrEntry->DdagNode->State,
                    StatusResponse);
            return STATUS_INTERNAL_ERROR;
    }

    if (NT_SUCCESS(Status))
    {
        if (LdrEntry->LoadContext)
        {
            RtlEnterCriticalSection(&LdrpLoaderLock);

            BOOLEAN HasInitializingNode = FALSE;
            Status = LdrpInitializeGraphRecurse(LdrEntry->DdagNode, StatusResponse, &HasInitializingNode);

            RtlLeaveCriticalSection(&LdrpLoaderLock);
        }
    }
    else
    {
        if (LdrpDebugFlags.LogWarning)
        {
            DPRINT1("LDR: %s([\"%wZ\"], ...): Load failed with 0x%08lX due to DLL load notifications or shimming\n",
                    __FUNCTION__, &LdrEntry->BaseDllName, Status);
        }

        if (LdrpDebugFlags.BreakInDebugger)
            __debugbreak();
    }

    return Status;
}
