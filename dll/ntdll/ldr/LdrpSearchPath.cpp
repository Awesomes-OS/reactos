#include <ldrp.h>
#include <reactos/ldrp.h>

UNICODE_STRING LdrpSeparatorSource = RTL_CONSTANT_STRING(L"\\");

NTSTATUS
NTAPI
LdrpSearchPath(IN PUNICODE_STRING DllName,
               IN PLDRP_PATH_SEARCH_CONTEXT PathSearchContext,
               IN BOOLEAN SearchInFirstSegmentOnly,
               IN OUT PLDRP_LAZY_PATH_EVALUATION_CONTEXT* LazyEvaluationContext OPTIONAL,
               IN OUT PLDRP_UNICODE_STRING_BUNDLE NtName,
               OUT PUNICODE_STRING BaseDllName,
               OUT PUNICODE_STRING FullDosName)
{
    NTSTATUS Status;
    PLDRP_LAZY_PATH_EVALUATION_CONTEXT CurrentLazyEvaluationContext = NULL;
    LDRP_LOAD_CONTEXT_FLAGS LoadContextFlags = { 0 };
    UNICODE_STRING Buffer = { 0 };
    SIZE_T MaxPathLength = 0u;

    /* Display debug output if snaps are on */
    if (LdrpDebugFlags.LogTrace)
    {
        DPRINT("LDR: %s(\"%wZ\", ...)\n", __FUNCTION__, DllName);
    }

    if (!PathSearchContext)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Skip;
    }

    if (LazyEvaluationContext)
        CurrentLazyEvaluationContext = *LazyEvaluationContext;

    if (!PathSearchContext->DllSearchPath.Length)
        PathSearchContext->DllSearchPath = LdrpDefaultPath;

    if (!CurrentLazyEvaluationContext)
    {
        CurrentLazyEvaluationContext = new LDRP_LAZY_PATH_EVALUATION_CONTEXT;
        CurrentLazyEvaluationContext->RemainingSearchPath = PathSearchContext->DllSearchPath;

        if (LazyEvaluationContext)
            *LazyEvaluationContext = CurrentLazyEvaluationContext;
    }

#ifndef NDEBUG
    {
        RTL_PATH_TYPE PathType = RtlDetermineDosPathNameType_U(DllName->Buffer);
        /* Check if we're dealing with a relative path */
        if (PathType != RtlPathTypeRelative)
        {
            BOOLEAN Exists = RtlDoesFileExists_UstrEx(DllName, TRUE);
            DPRINT1("LDR: %s shouldn't be called with non-relative [%u] path [%wZ], Exists=[%u]\n",
                __FUNCTION__, PathType, DllName, Exists);
        }
    }
#endif

    Status = STATUS_DLL_NOT_FOUND;

    {
        WCHAR *CurrentStart = CurrentLazyEvaluationContext->RemainingSearchPath.Buffer;
        WCHAR *CurrentIterator = CurrentStart;
        SIZE_T DllSearchPathIndex = 0;
        ASSERT(CurrentIterator);
        for (; DllSearchPathIndex < CurrentLazyEvaluationContext->RemainingSearchPath.Length;
               ++CurrentIterator, ++DllSearchPathIndex)
        {
            WCHAR c = *CurrentIterator;
            if (c == UNICODE_NULL)
                break;

            if (c == L';')
            {
                MaxPathLength = max(MaxPathLength, (CurrentIterator - CurrentStart));
                CurrentStart = CurrentIterator + 1;

                if (SearchInFirstSegmentOnly)
                    break;
            }
        }
        MaxPathLength = max(MaxPathLength, (CurrentIterator - CurrentStart));
    }

    if (MaxPathLength != 0)
    {
        // SearchPath[:] .. "\" .. DllName
        const SIZE_T Length = (MaxPathLength + 1) * sizeof(WCHAR) + DllName->Length;

        if (!NT_SUCCESS(Status = LdrpAllocateUnicodeString(&Buffer, Length)))
        {
            goto Skip;
        }
    }

    __try
    {
        UNICODE_STRING PathSource;

        if (MaxPathLength == 0)
        {
            Status = STATUS_DLL_NOT_FOUND;
            _SEH2_LEAVE;
        }

        PathSource.Buffer = CurrentLazyEvaluationContext->RemainingSearchPath.Buffer;
        WCHAR *CurrentIterator = PathSource.Buffer;
        for (;; ++CurrentIterator)
        {
            WCHAR c = *CurrentIterator;
            if (c != L';' && c != UNICODE_NULL)
                continue;

            PathSource.Length = (CurrentIterator - PathSource.Buffer) * sizeof(WCHAR);
            PathSource.MaximumLength = PathSource.Length + sizeof(WCHAR);

            CurrentLazyEvaluationContext->RemainingSearchPath.Buffer = CurrentIterator + 1;
            CurrentLazyEvaluationContext->RemainingSearchPath.Length -= PathSource.MaximumLength;
            CurrentLazyEvaluationContext->RemainingSearchPath.MaximumLength -= PathSource.MaximumLength;

            if (PathSource.Length)
            {
                ASSERT(NT_SUCCESS(RtlAppendUnicodeStringToString(&Buffer, &PathSource)));

                WCHAR *LastPathChar = Buffer.Buffer + Buffer.Length / sizeof(WCHAR) - 1;

                if (*LastPathChar != L'\\' && *LastPathChar != L'/')
                    ASSERT(NT_SUCCESS(RtlAppendUnicodeStringToString(&Buffer, &LdrpSeparatorSource)));

                ASSERT(NT_SUCCESS(RtlAppendUnicodeStringToString(&Buffer, DllName)));

                Status = LdrpResolveDllName(&Buffer, NtName, BaseDllName, FullDosName, LoadContextFlags);

                if (Status != STATUS_DLL_NOT_FOUND && Status != STATUS_ACCESS_DENIED && Status != STATUS_SHARING_VIOLATION && Status != STATUS_LOGON_FAILURE)
                {
                    // todo: verify if we are allowed to use arbitrary location, if not,
                    // iterate through RtlpLookupSafeCurDirList() and compare prefixes
                    _SEH2_LEAVE;
                }

                NtName->String.Length = 0;
                RtlEraseUnicodeString(&Buffer);
            }

            if (SearchInFirstSegmentOnly)
                break;

            PathSource.Buffer = CurrentLazyEvaluationContext->RemainingSearchPath.Buffer;

            if (c == UNICODE_NULL)
                break;
        }

        Status = STATUS_DLL_NOT_FOUND;
    }
    _SEH2_FINALLY
    {
        LdrpFreeUnicodeString(&Buffer);
    }
    _SEH2_END

Skip:

    /* Display debug output if snaps are on */
    if (LdrpDebugFlags.LogTrace)
    {
        /* Check which output to use -- failure or success */
        if (NT_SUCCESS(Status))
        {
            DPRINT("LDR: %s(...) -> DOS: \"%wZ\", NT: \"%wZ\"\n", __FUNCTION__, FullDosName, &NtName->String);
        }
        else
        {
            DPRINT("LDR: %s(\"%wZ\", ...) -> 0x%08lX\n", __FUNCTION__, DllName, Status);
        }
    }

    return Status;
}
