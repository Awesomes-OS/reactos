#include <ldrp.h>
#include <ndk/mmfuncs.h>

NTSTATUS
NTAPI
LdrpDoPostSnapWork(IN PLDRP_LOAD_CONTEXT LoadContext)
{
    NTSTATUS Status = STATUS_SUCCESS;

    PVOID BaseAddress = LoadContext->IATBaseAddress;

    if (BaseAddress)
    {
        SIZE_T IATSize = LoadContext->IATSize;
        ULONG GoodOldNewProtection; // which we don't care about
        Status = NtProtectVirtualMemory(NtCurrentProcess(),
                                        &BaseAddress,
                                        &IATSize,
                                        LoadContext->IATOriginalProtection,
                                        &GoodOldNewProtection);

        /* Also flush out the cache */
        NtFlushInstructionCache(NtCurrentProcess(), LoadContext->IATBaseAddress, LoadContext->IATSize);
    }

    if (NT_SUCCESS(Status))
    {
        const PVOID GuardCFCheckFunctionPointer = LoadContext->GuardCFCheckFunctionPointer;
        PVOID* GuardCFCheckFunctionPointerThunk = LoadContext->GuardCFCheckFunctionPointerThunk;

        if (GuardCFCheckFunctionPointer && *GuardCFCheckFunctionPointerThunk != GuardCFCheckFunctionPointer)
            __fastfail(FAST_FAIL_MRDATA_MODIFIED);

        if (!LoadContext->Entry->TlsIndex) // if not initialized (either just loaded or having no TLS data)
            Status = LdrpHandleTlsData(LoadContext->Entry);
    }

    if (!NT_SUCCESS(Status))
    {
        if (LdrpDebugFlags.BreakInDebugger)
            __debugbreak();
    }

    return Status;
}
