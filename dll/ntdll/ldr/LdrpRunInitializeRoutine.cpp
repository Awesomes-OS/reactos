#include <ldrp.h>

EXTERN_C
{

#include <psdk/winreg.h>

}

NTSTATUS
NTAPI
LdrpRunInitializeRoutine(IN PLDR_DATA_TABLE_ENTRY Module)
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
        Status = LdrQueryImageFileExecutionOptions(&Module->BaseDllName,
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
        if (LdrpDebugFlags.LogInformation)
        {
            DPRINT1("LDR: %wZ loaded.", &Module->BaseDllName);
            DPRINT1(" - About to call init routine at " LDR_HEXPTR_FMT "\n", Module->EntryPoint);
        }

        /* Break in debugger */
        DbgBreakPoint();
    }

    /* Display debug message */
    if (LdrpDebugFlags.LogTrace)
    {
        DPRINT1("[" LDR_HEXPTR_FMT "," LDR_HEXPTR_FMT "] LDR: %wZ init routine " LDR_HEXPTR_FMT "\n",
                NtCurrentTeb()->RealClientId.UniqueThread,
                NtCurrentTeb()->RealClientId.UniqueProcess,
                &Module->FullDllName,
                Module->EntryPoint);
    }

    /* Save the old Dll Initializer and write the current one */
    const PLDR_DATA_TABLE_ENTRY OldInitializer = LdrpCurrentDllInitializer;
    LdrpCurrentDllInitializer = Module;

    /* Set up the Act Ctx */
    ActCtx.Size = sizeof(ActCtx);
    ActCtx.Format = 1;
    RtlZeroMemory(&ActCtx.Frame, sizeof(RTL_ACTIVATION_CONTEXT_STACK_FRAME));

    /* Activate the ActCtx */
    RtlActivateActivationContextUnsafeFast(&ActCtx,
                                           Module->EntryPointActivationContext);

    __try
    {
        /* Check if it has TLS */
        if (Module->TlsIndex)
        {
            /* Call TLS */
            LdrpCallTlsInitializers(Module, DLL_PROCESS_ATTACH);
        }


        /* Make sure we have an entrypoint */
        if (Module->EntryPoint)
        {
            const PCONTEXT Context = Module->ProcessStaticImport
                                         ? LdrpProcessInitContextRecord
                                         : NULL;

            /* Call the Entrypoint */
            if (LdrpDebugFlags.LogTrace)
            {
                DPRINT1("%wZ - Calling entry point at " LDR_HEXPTR_FMT " for DLL_PROCESS_ATTACH\n",
                        &Module->BaseDllName, Module->EntryPoint);
            }

            DllStatus = LdrpCallInitRoutine((PDLL_INIT_ROUTINE)Module->EntryPoint,
                                            Module->DllBase,
                                            DLL_PROCESS_ATTACH,
                                            Context);
        }
    }
    __except(LdrpDebugExceptionFilter(GetExceptionCode(), GetExceptionInformation()))
    {
        DllStatus = FALSE;
        DPRINT1("WARNING: Exception 0x%08lX during LdrpCallInitRoutine(DLL_PROCESS_ATTACH) for %wZ\n",
                GetExceptionCode(), &Module->BaseDllName);
    }

    /* Deactivate the ActCtx */
    RtlDeactivateActivationContextUnsafeFast(&ActCtx);

    /* Save the Current DLL Initializer */
    LdrpCurrentDllInitializer = OldInitializer;

    /* Mark the entry as processed */
    if (Module->EntryPoint)
        Module->ProcessAttachCalled = TRUE;

    /* Fail if DLL init failed */
    if (!DllStatus)
    {
        DPRINT1("LDR: DLL_PROCESS_ATTACH for dll \"%wZ\" (InitRoutine: " LDR_HEXPTR_FMT ") failed\n",
                &Module->BaseDllName, Module->EntryPoint);

        if (LdrpDebugFlags.BreakInDebugger)
            __debugbreak();

        Module->ProcessAttachFailed = TRUE;
        Status = STATUS_DLL_INIT_FAILED;
    }

Quickie:
    /* Return to caller */
    DPRINT("LdrpRunInitializeRoutine(%wZ) done\n", &Module->BaseDllName);
    return Status;
}
