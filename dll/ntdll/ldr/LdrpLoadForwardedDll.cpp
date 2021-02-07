#include <ldrp.h>

NTSTATUS
NTAPI
LdrpLoadForwardedDll(IN PANSI_STRING RawDllName,
                     IN PLDRP_PATH_SEARCH_CONTEXT PathSearchContext,
                     IN PLDR_DATA_TABLE_ENTRY ForwarderSource,
                     IN PLDR_DATA_TABLE_ENTRY ParentEntry,
                     IN LDR_DLL_LOAD_REASON LoadReason,
                     OUT PLDR_DATA_TABLE_ENTRY* OutputLdrEntry)
{
    NTSTATUS Status;
    LDRP_UNICODE_STRING_BUNDLE DllName, RawDllNameUnicode;
    LDRP_LOAD_CONTEXT_FLAGS Flags = {0};

    LdrpCreateUnicodeStringBundle(DllName);
    LdrpCreateUnicodeStringBundle(RawDllNameUnicode);

    Status = LdrpAppendAnsiStringToFilenameBuffer(&RawDllNameUnicode, RawDllName);
    if (!NT_SUCCESS(Status))
        goto Quickie;

    RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_EXTENDED ActCtx;

    // Set up and activate the Activation Context based upon ParentEntry
    RtlZeroMemory(&ActCtx, sizeof(ActCtx));
    ActCtx.Size = sizeof(ActCtx);
    ActCtx.Format = RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_FORMAT_WHISTLER;

    RtlActivateActivationContextUnsafeFast(&ActCtx,
                                           ParentEntry->EntryPointActivationContext);

    Status = LdrpPreprocessDllName(&RawDllNameUnicode.String, &DllName, ParentEntry, &Flags);

    if (NT_SUCCESS(Status))
    {
        LdrpLoadDllInternal(&DllName.String,
                            PathSearchContext,
                            Flags,
                            LoadReason,
                            ParentEntry,
                            ForwarderSource,
                            OutputLdrEntry,
                            &Status);
    }

    RtlDeactivateActivationContextUnsafeFast(&ActCtx);

Quickie:

    LdrpFreeUnicodeStringBundle(DllName);
    LdrpFreeUnicodeStringBundle(RawDllNameUnicode);

    return Status;
}
