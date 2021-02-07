#include <ldrp.h>
#include <reactos/ldrp.h>

NTSTATUS
NTAPI
LdrpMapDllSearchPath(IN PLDRP_LOAD_CONTEXT LoadContext)
{
    LDRP_UNICODE_STRING_BUNDLE FullNtDllPath;
    UNICODE_STRING BaseDllName, FullDosDllPath;
    BOOLEAN FoundAtLeastOne = FALSE;
    PLDR_DATA_TABLE_ENTRY Module = NULL, LoadEntry = LoadContext->Module;
    NTSTATUS Status;
    PLDRP_LAZY_PATH_EVALUATION_CONTEXT LazyEvaluationContext = NULL;

    LdrpCreateUnicodeStringBundle(FullNtDllPath);
    RtlInitEmptyUnicodeString(&FullDosDllPath, NULL, 0);

    // Repeat until exhausted the whole PATH or found PE image for current machine platform
    while (TRUE)
    {
#ifndef NDEBUG
        RtlZeroMemory(&BaseDllName, sizeof(UNICODE_STRING));
#endif

        Status = LdrpSearchPath(&LoadContext->DllName,
                                LoadContext->PathSearchContext,
                                LoadContext->Flags.SearchOnlyFirstPathSegment,
                                &LazyEvaluationContext,
                                &FullNtDllPath,
                                &BaseDllName,
                                &FullDosDllPath);

        if (!NT_SUCCESS(Status))
        {
            if (Status == STATUS_DLL_NOT_FOUND && FoundAtLeastOne)
                Status = STATUS_INVALID_IMAGE_FORMAT;

            if (Status != STATUS_DLL_NOT_FOUND)
                break;
        }

        FoundAtLeastOne = TRUE;

        Status = LdrpAppCompatRedirect(LoadContext, &FullDosDllPath, &BaseDllName, &FullNtDllPath, Status);

        if (!NT_SUCCESS(Status))
            break;

        const ULONG32 BaseNameHashValue = LdrpHashUnicodeString(&BaseDllName);
        LoadEntry->BaseNameHashValue = BaseNameHashValue;

        Status = LdrpFindExistingModule(&BaseDllName, &FullDosDllPath, LoadContext->Flags, BaseNameHashValue, &Module);

        if (Status != STATUS_DLL_NOT_FOUND)
            break;

        LdrpFreeUnicodeString(&LoadEntry->FullDllName);
        LoadEntry->BaseDllName = BaseDllName;
        LoadEntry->FullDllName = FullDosDllPath;

        RtlInitEmptyUnicodeString(&FullDosDllPath, NULL, 0);

        /* Do the heavy work */
        Status = LdrpMapDllNtFileName(LoadContext, &FullNtDllPath.String);

        if (Status != STATUS_IMAGE_MACHINE_TYPE_MISMATCH)
            break;

        LdrpFreeUnicodeStringBundle(FullNtDllPath);
    }

    if (Module)
    {
        LdrpLoadContextReplaceModule(LoadContext, Module);
    }

    LdrpFreeUnicodeStringBundle(FullNtDllPath);

    LdrpFreeUnicodeString(&FullDosDllPath);

    return Status;
}
