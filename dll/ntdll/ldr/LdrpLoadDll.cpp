#include <ldrp.h>

NTSTATUS
NTAPI
LdrpLoadDll(IN PUNICODE_STRING RawDllName,
            IN PLDRP_PATH_SEARCH_CONTEXT PathSearchContext,
            IN LDRP_LOAD_CONTEXT_FLAGS LoaderFlags,
            OUT PLDR_DATA_TABLE_ENTRY *OutputLdrEntry)
{
    NTSTATUS Status;
    LDRP_UNICODE_STRING_BUNDLE DllName;

    LdrpCreateUnicodeStringBundle(DllName);

    Status = LdrpPreprocessDllName(RawDllName, &DllName, NULL, &LoaderFlags);

    /* Check success */
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("LDR: LdrpPreprocessDllName(\"%wZ\") -> 0x%08lX\n",
                RawDllName, Status);
        goto Quickie;
    }

    LdrpLoadDllInternal(&DllName.String,
                        PathSearchContext,
                        LoaderFlags,
                        LoadReasonDynamicLoad,
                        NULL,
                        NULL,
                        OutputLdrEntry,
                        &Status);

Quickie:

    LdrpFreeUnicodeStringBundle(DllName);

    return Status;
}