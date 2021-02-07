#include <ldrp.h>
#include <reactos/ldrp.h>
#include <ndk/ntndk.h>

NTSTATUS
NTAPI
LdrpPreprocessDllName(IN PUNICODE_STRING DllName,
                      IN OUT PLDRP_UNICODE_STRING_BUNDLE OutputDllName,
                      IN PLDR_DATA_TABLE_ENTRY ParentEntry OPTIONAL,
                      OUT PLDRP_LOAD_CONTEXT_FLAGS LoadContextFlags)
{
    BOOLEAN RedirectedSxS = FALSE;
    NTSTATUS Status;

    ASSERT(DllName && OutputDllName && LoadContextFlags);

    if (!NT_SUCCESS(Status = LdrpApplyFileNameRedirection(DllName, OutputDllName, &RedirectedSxS)))
    {
        goto Quickie;
    }

    if (OutputDllName->String.Length)
    {
        DPRINT1("LDR: %s(\"%wZ\", ...): Redirected to %wZ by %s\n",
                __FUNCTION__,
                DllName,
                OutputDllName,
                RedirectedSxS ? "SxS" : "API set");

        LoadContextFlags->Redirected = TRUE;
        LoadContextFlags->HasFullPath = TRUE;

        if (!RedirectedSxS)
            LoadContextFlags->RedirectedByAPISet = TRUE;
    }
    else
    {
        if (LdrpIsBaseNameOnly(DllName))
        {
            Status = LdrpAppendUnicodeStringToFilenameBuffer(OutputDllName, DllName);
            LoadContextFlags->BaseNameOnly = TRUE;
        }
        else if (RtlDetermineDosPathNameType_Ustr(DllName) == RtlPathTypeRelative)
        {
            Status = LdrpAppendUnicodeStringToFilenameBuffer(OutputDllName, DllName);
        }
        else
        {
            if (NT_SUCCESS(Status = LdrpGetFullPath(DllName, OutputDllName)))
            {
                LoadContextFlags->HasFullPath = TRUE;
                LoadContextFlags->RetryDependentLoadWhenMissing = TRUE;
            }
        }
    }

Quickie:

    if (NT_SUCCESS(Status))
    {
        /* If no extension was found, add the default extension */
        if (!LdrpIsExtensionPresent(&OutputDllName->String))
        {
            Status = LdrpAppendUnicodeStringToFilenameBuffer(OutputDllName, &LdrpDefaultExtension);
        }
        else
        {
            LdrpTrimTrailingDots(&OutputDllName->String);
        }

        if (LdrpDebugFlags.LogTrace)
            DPRINT1("LDR: %s(\"%wZ\", ...) -> \"%wZ\" | 0x%08lX\n", __FUNCTION__, DllName, OutputDllName, Status);
    }

    if (LdrpDebugFlags.LogWarning && !NT_SUCCESS(Status))
    {
        DPRINT1("LDR: %s(\"%wZ\", ...) -> 0x%08lX\n", __FUNCTION__, DllName, Status);
    }

    if (LdrpDebugFlags.BreakInDebugger && !NT_SUCCESS(Status))
    {
        __debugbreak();
    }

    return Status;
}
