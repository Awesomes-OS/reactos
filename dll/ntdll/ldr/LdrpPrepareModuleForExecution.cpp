#include <ldrp.h>

NTSTATUS
NTAPI
LdrpPrepareModuleForExecution(IN PLDR_DATA_TABLE_ENTRY Module,
                              IN NTSTATUS* StatusResponse)
{
    NTSTATUS Status = STATUS_SUCCESS;

    LDRP_ASSERT_MODULE_ENTRY(Module);

    switch (Module->DdagNode->State)
    {
        case LdrModulesSnapped:
            LdrpCondenseGraph(Module->DdagNode);
            ASSERT(Module->DdagNode->State == LdrModulesCondensed);
            // fallthrough
        case LdrModulesCondensed:
            if (!Module->ProcessStaticImport && NtCurrentTeb()->SubProcessTag)
            {
                DPRINT1("LDR: %s(" LDR_HEXPTR_FMT "[%wZ:%d], ...): tagging not implemented\n",
                        __FUNCTION__,
                        Module, &Module->BaseDllName, Module->DdagNode->State);
                // todo: LdrpAddNodeServiceTag
            }

            Status = LdrpNotifyLoadOfGraph(Module->DdagNode);
            if (NT_SUCCESS(Status))
            {
                ASSERT(Module->DdagNode->State == LdrModulesReadyToInit);
                // todo: LdrpDynamicShimModule
            }
            // fallthrough
        case LdrModulesReadyToInit:
            break;

        case LdrModulesInitializing:
        case LdrModulesReadyToRun:
            return STATUS_SUCCESS;

        default:
            DPRINT1("%s(" LDR_HEXPTR_FMT "[%wZ:%d], " LDR_HEXPTR_FMT "): unexpected state, aborting.\n",
                    __FUNCTION__,
                    Module, &Module->BaseDllName, Module->DdagNode->State,
                    StatusResponse);
            return STATUS_INTERNAL_ERROR;
    }

    if (NT_SUCCESS(Status))
    {
        const PLDRP_LOAD_CONTEXT LoadContext = Module->LoadContext;

        LDRP_ASSERT_LOAD_CONTEXT_OPTIONAL(LoadContext);

        if (LoadContext && !LoadContext->Flags.ProcessImage)
        {
            RtlEnterCriticalSection(&LdrpLoaderLock);

            BOOLEAN HasInitializingNode = FALSE;
            Status = LdrpInitializeGraphRecurse(Module->DdagNode, StatusResponse, &HasInitializingNode);

            RtlLeaveCriticalSection(&LdrpLoaderLock);
        }
    }
    else
    {
        if (LdrpDebugFlags.LogWarning)
        {
            DPRINT1("LDR: %s([\"%wZ\"], ...): Load failed with 0x%08lX due to DLL load notifications or shimming\n",
                    __FUNCTION__, &Module->BaseDllName, Status);
        }

        if (LdrpDebugFlags.BreakInDebugger)
            __debugbreak();
    }

    return Status;
}
