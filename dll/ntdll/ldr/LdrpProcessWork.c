#include <ldrp.h>
#include <ndk/exfuncs.h>

void
NTAPI
LdrpProcessWork(PLDRP_LOAD_CONTEXT LoadContext)
{
    NTSTATUS Status = *LoadContext->StatusResponse;

    if (!NT_SUCCESS(Status))
    {
        __debugbreak();
    }

    ASSERT(LoadContext->Entry);

    if (LoadContext->Entry->DdagNode->State)
    {
        ASSERT(LoadContext->Entry->DdagNode->State >= LdrModulesMapped);
        Status = LdrpSnapModule(LoadContext);
    }
    else
    {
        if (LoadContext->Flags.HasFullPath)
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
            DPRINT1("LDR: Unable to load DLL: \"%wZ\", Parent Module: \"%wZ\", Status: 0x%08lX\n",
                    &LoadContext->DllName,
                    LoadContext->ParentEntry ? &LoadContext->ParentEntry->FullDllName : NULL,
                    Status);

            if (LdrpDebugFlags.BreakInDebugger)
                __debugbreak();

            /* We couldn't map, is this a static load? */
            if (LoadContext->Entry->ProcessStaticImport)
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
                HardErrorParameters[0] = (ULONG_PTR)& HardErrorDllName;

                /* Raise the hard error */
                NtRaiseHardError(STATUS_DLL_NOT_FOUND,
                    1,
                    0x00000001,
                    HardErrorParameters,
                    OptionOk,
                    &Response);

                /* We're back, where we initializing? */
                if (LdrpInLdrInit) LdrpFatalHardErrorCount++;
            }
        }
    }

    if (!NT_SUCCESS(Status))
        *LoadContext->StatusResponse = Status;
}