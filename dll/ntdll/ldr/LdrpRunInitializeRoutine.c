#include <ldrp.h>
#include <psdk/winreg.h>

NTSTATUS
NTAPI
LdrpRunInitializeRoutine(IN PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    NTSTATUS Status = STATUS_SUCCESS;

    const PPEB Peb = NtCurrentPeb();
    RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_EXTENDED ActCtx;
    ULONG BreakOnDllLoad;
    BOOLEAN DllStatus = TRUE;

    /* Are we being debugged? */
    BreakOnDllLoad = 0;
    if (Peb->BeingDebugged || Peb->ReadImageFileExecOptions)
    {
        /* Check if we should break on load */
        Status = LdrQueryImageFileExecutionOptions(&LdrEntry->BaseDllName,
                                                   L"BreakOnDllLoad",
                                                   REG_DWORD,
                                                   &BreakOnDllLoad,
                                                   sizeof(ULONG),
                                                   NULL);
        if (!NT_SUCCESS(Status))
            BreakOnDllLoad = 0;

        /* Reset status back to STATUS_SUCCESS */
        Status = STATUS_SUCCESS;
    }

    /* Break if asked */
    if (BreakOnDllLoad)
    {
        /* Check if we should show a message */
        if (ShowSnaps)
        {
            DPRINT1("LDR: %wZ loaded.", &LdrEntry->BaseDllName);
            DPRINT1(" - About to call init routine at %p\n", LdrEntry->EntryPoint);
        }

        /* Break in debugger */
        DbgBreakPoint();
    }

    /* Display debug message */
    if (ShowSnaps)
    {
        DPRINT1("[%p,%p] LDR: %wZ init routine %p\n",
                NtCurrentTeb()->RealClientId.UniqueThread,
                NtCurrentTeb()->RealClientId.UniqueProcess,
                &LdrEntry->FullDllName,
                LdrEntry->EntryPoint);
    }

    /* Save the old Dll Initializer and write the current one */
    const PLDR_DATA_TABLE_ENTRY OldInitializer = LdrpCurrentDllInitializer;
    LdrpCurrentDllInitializer = LdrEntry;

    /* Set up the Act Ctx */
    ActCtx.Size = sizeof(ActCtx);
    ActCtx.Format = 1;
    RtlZeroMemory(&ActCtx.Frame, sizeof(RTL_ACTIVATION_CONTEXT_STACK_FRAME));

    /* Activate the ActCtx */
    RtlActivateActivationContextUnsafeFast(&ActCtx,
                                           LdrEntry->EntryPointActivationContext);

    _SEH2_TRY
    {
        /* Check if it has TLS */
        if (LdrEntry->TlsIndex)
        {
            /* Call TLS */
            LdrpCallTlsInitializers(LdrEntry, DLL_PROCESS_ATTACH);
        }


        /* Make sure we have an entrypoint */
        if (LdrEntry->EntryPoint)
        {
            const PCONTEXT Context = LdrEntry->ProcessStaticImport
                                         ? LdrpProcessInitContextRecord
                                         : 0;

            /* Call the Entrypoint */
            if (ShowSnaps)
            {
                DPRINT1("%wZ - Calling entry point at %p for DLL_PROCESS_ATTACH\n",
                        &LdrEntry->BaseDllName, LdrEntry->EntryPoint);
            }

            DllStatus = LdrpCallInitRoutine(LdrEntry->EntryPoint,
                                            LdrEntry->DllBase,
                                            DLL_PROCESS_ATTACH,
                                            Context);
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        DllStatus = FALSE;
        DPRINT1("WARNING: Exception 0x%08lX during LdrpCallInitRoutine(DLL_PROCESS_ATTACH) for %wZ\n",
                _SEH2_GetExceptionCode(), &LdrEntry->BaseDllName);
    }
    _SEH2_END;

    /* Deactivate the ActCtx */
    RtlDeactivateActivationContextUnsafeFast(&ActCtx);

    /* Save the Current DLL Initializer */
    LdrpCurrentDllInitializer = OldInitializer;

    /* Mark the entry as processed */
    if (LdrEntry->EntryPoint)
        LdrEntry->ProcessAttachCalled = TRUE;

    /* Fail if DLL init failed */
    if (!DllStatus)
    {
        DPRINT1("LDR: DLL_PROCESS_ATTACH for dll \"%wZ\" (InitRoutine: %p) failed\n",
                &LdrEntry->BaseDllName, LdrEntry->EntryPoint);

        if (LdrpDebugFlags.BreakInDebugger)
            __debugbreak();

        LdrEntry->ProcessAttachFailed = TRUE;
        Status = STATUS_DLL_INIT_FAILED;
    }

Quickie:
    /* Return to caller */
    DPRINT("LdrpRunInitializeRoutine(%wZ) done\n", &LdrEntry->BaseDllName);
    return Status;
}