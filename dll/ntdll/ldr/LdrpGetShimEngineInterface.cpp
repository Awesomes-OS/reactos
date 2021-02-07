#include <ldrp.h>

PVOID LdrpGetShimEngineFunction(PCSZ FunctionName)
{
    ANSI_STRING Function;
    NTSTATUS Status;
    PVOID Address;
    RtlInitAnsiString(&Function, FunctionName);
    /* Skip Dll init */
    Status = LdrGetProcedureAddressEx(g_pShimEngineModule, &Function, 0, &Address, 0);
    return NT_SUCCESS(Status) ? Address : NULL;
}

VOID
NTAPI
LdrpGetShimEngineInterface()
{
    PVOID SE_DllLoaded = LdrpGetShimEngineFunction("SE_DllLoaded");
    PVOID SE_DllUnloaded = LdrpGetShimEngineFunction("SE_DllUnloaded");
    PVOID SE_InstallBeforeInit = LdrpGetShimEngineFunction("SE_InstallBeforeInit");
    PVOID SE_InstallAfterInit = LdrpGetShimEngineFunction("SE_InstallAfterInit");
    PVOID SE_ProcessDying = LdrpGetShimEngineFunction("SE_ProcessDying");
    PVOID SE_LdrResolveDllName = LdrpGetShimEngineFunction("SE_LdrResolveDllName");

    if (SE_DllLoaded && SE_DllUnloaded && SE_InstallBeforeInit && SE_InstallAfterInit && SE_ProcessDying)
    {
        g_pfnSE_DllLoaded = RtlEncodeSystemPointer(SE_DllLoaded);
        g_pfnSE_DllUnloaded = RtlEncodeSystemPointer(SE_DllUnloaded);
        g_pfnSE_InstallBeforeInit = RtlEncodeSystemPointer(SE_InstallBeforeInit);
        g_pfnSE_InstallAfterInit = RtlEncodeSystemPointer(SE_InstallAfterInit);
        g_pfnSE_ProcessDying = RtlEncodeSystemPointer(SE_ProcessDying);

        // Mandatory on Windows, optional on ReactOS
        g_pfnSE_LdrResolveDllName = SE_LdrResolveDllName ? RtlEncodeSystemPointer(SE_LdrResolveDllName) : NULL;

        g_ShimsEnabled = TRUE;
    }
    else
    {
        LdrpUnloadShimEngine();
    }
}
