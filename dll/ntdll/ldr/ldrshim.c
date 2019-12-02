#include <ldrp.h>

VOID
NTAPI
LdrpRunShimEngineInitRoutine(IN ULONG Reason)
{
    PLIST_ENTRY ListHead, Next;
    PLDR_DATA_TABLE_ENTRY LdrEntry;

    ListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
    Next = ListHead->Flink;
    while (Next != ListHead)
    {
        LdrEntry = CONTAINING_RECORD(Next, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

        if (g_pShimEngineModule == LdrEntry->DllBase)
        {
            if (LdrEntry->EntryPoint)
            {
                _SEH2_TRY
                {
                    LdrpCallInitRoutine(LdrEntry->EntryPoint, LdrEntry->DllBase, Reason, NULL);
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    DPRINT1("WARNING: Exception 0x%x during LdrpRunShimEngineInitRoutine(%u)\n",
                            _SEH2_GetExceptionCode(), Reason);
                }
                _SEH2_END;
            }
            return;
        }

        Next = Next->Flink;
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
            VOID(NTAPI *SE_InstallBeforeInit)(PUNICODE_STRING, PVOID);
            SE_InstallBeforeInit = RtlDecodeSystemPointer(g_pfnSE_InstallBeforeInit);
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