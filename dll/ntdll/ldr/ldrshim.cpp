#include <ldrp.h>

VOID
NTAPI
LdrpRunShimEngineInitRoutine(IN ULONG Reason)
{
    const PLIST_ENTRY ListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
    for (PLIST_ENTRY Next = ListHead->Flink; Next != ListHead; Next = Next->Flink)
    {
        PLDR_DATA_TABLE_ENTRY Module = CONTAINING_RECORD(Next, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

        if (g_pShimEngineModule != Module->DllBase)
            continue;

        if (Module->EntryPoint)
        {
            __try
            {
                LdrpCallInitRoutine((PDLL_INIT_ROUTINE)Module->EntryPoint, Module->DllBase, Reason, NULL);
            }
            __except(LdrpDebugExceptionFilter(GetExceptionCode(), GetExceptionInformation()))
            {
                DPRINT1("WARNING: Exception 0x%x during LdrpRunShimEngineInitRoutine(%u)\n",
                        GetExceptionCode(), Reason);
            }
        }

        return;
    }
}

VOID
NTAPI
LdrpLoadShimEngine(IN PWSTR ImageName, IN PUNICODE_STRING ProcessImage, IN PVOID pShimData)
{
    UNICODE_STRING ShimLibraryName;
    PVOID ShimLibrary;
    NTSTATUS Status;
    RtlInitUnicodeString(&ShimLibraryName, ImageName);
    // todo: andrew.boyarshin: upstream has modified this to pass CallInit = FALSE. Resolve this.
    /* We should NOT pass CallInit = TRUE!
       If we do this, other init routines will be called before we get a chance to shim stuff.. */
    Status = LdrLoadDll(NULL, NULL, &ShimLibraryName, &ShimLibrary);
    if (NT_SUCCESS(Status))
    {
        g_pShimEngineModule = ShimLibrary;
        LdrpRunShimEngineInitRoutine(DLL_PROCESS_ATTACH);
        LdrpGetShimEngineInterface();
        if (g_ShimsEnabled)
        {
            void (NTAPI *SE_InstallBeforeInit)(PUNICODE_STRING, PVOID) = static_cast<decltype(SE_InstallBeforeInit)>(
                RtlDecodeSystemPointer(g_pfnSE_InstallBeforeInit)
            );

            SE_InstallBeforeInit(ProcessImage, pShimData);
        }
    }
}

VOID
NTAPI
LdrpUnloadShimEngine()
{
    /* Make sure we do not call into the shim engine anymore */
    g_ShimsEnabled = FALSE;
    LdrpRunShimEngineInitRoutine(DLL_PROCESS_DETACH);
    LdrUnloadDll(g_pShimEngineModule);
    g_pShimEngineModule = NULL;
}
