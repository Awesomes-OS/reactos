#include <ldrp.h>
#include <ndk/ntndk.h>
#include <reactos/ldrp.h>

NTSTATUS
NTAPI
LdrpFindLoadedDllInternal(PUNICODE_STRING DllName,
                          PLDRP_PATH_SEARCH_CONTEXT PathSearchContext,
                          PLDR_DATA_TABLE_ENTRY *Module,
                          LDR_DDAG_STATE *DdagState,
                          LDRP_LOAD_CONTEXT_FLAGS Flags)
{
    NTSTATUS Status = STATUS_DLL_NOT_FOUND;
    *Module = NULL;

    DPRINT("LDR: %s(\"%wZ\", " LDR_HEXPTR_FMT ", ..., %#X)\n", __FUNCTION__, DllName, PathSearchContext, Flags.Flags);

    if (Flags.BaseNameOnly)
    {
        Status = LdrpFindLoadedDllByName(DllName, NULL, Flags, Module, DdagState);
    }
    else if (Flags.HasFullPath)
    {
        Status = LdrpFindLoadedDllByName(NULL, DllName, Flags, Module, DdagState);
    }

    if (!Flags.BaseNameOnly && !NT_SUCCESS(Status))
    {
        UNICODE_STRING FullDosDllPath = {0}, BaseDllName = {0};
        LDRP_UNICODE_STRING_BUNDLE NtName;

        LdrpCreateUnicodeStringBundle(NtName);

        if (Flags.HasFullPath)
        {
            Status = LdrpResolveDllName(DllName, &NtName, &BaseDllName, &FullDosDllPath, Flags);
        }
        else
        {
            Status = LdrpSearchPath(DllName,
                                    PathSearchContext,
                                    FALSE,
                                    NULL,
                                    &NtName,
                                    &BaseDllName,
                                    &FullDosDllPath);
        }

        if (NT_SUCCESS(Status))
        {
            Status = LdrpFindLoadedDllByName(&BaseDllName, &FullDosDllPath, Flags, Module, DdagState);

            if (Status == STATUS_DLL_NOT_FOUND)
            {
                Status = LdrpFindLoadedDllByMappingFile(&NtName.String, Module, DdagState);
            }
        }

        LdrpFreeUnicodeString(&FullDosDllPath);
        LdrpFreeUnicodeStringBundle(NtName);
    }

    if (DdagState)
        DPRINT("LDR: %s(\"%wZ\", " LDR_HEXPTR_FMT ", " LDR_HEXPTR_FMT ", %d, %#X) -> 0x%08lX\n",
               __FUNCTION__,
               DllName,
               PathSearchContext,
               *Module,
               *DdagState,
               Flags,
               Status);
    else
        DPRINT("LDR: %s(\"%wZ\", " LDR_HEXPTR_FMT ", " LDR_HEXPTR_FMT ", NULL, %#X) -> 0x%08lX\n",
               __FUNCTION__,
               DllName,
               PathSearchContext,
               *Module,
               Flags,
               Status);

    return Status;
}

NTSTATUS
NTAPI
LdrpFindLoadedDll(PUNICODE_STRING RawDllName,
                  PLDRP_PATH_SEARCH_CONTEXT PathSearchContext,
                  PLDR_DATA_TABLE_ENTRY *Module)
{
    NTSTATUS Status;
    LDRP_UNICODE_STRING_BUNDLE DllName;
    LDRP_LOAD_CONTEXT_FLAGS NewContextFlags = {0};

    LdrpCreateUnicodeStringBundle(DllName);

    Status = LdrpPreprocessDllName(RawDllName, &DllName, NULL, &NewContextFlags);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("LDR: LdrpPreprocessDllName(\"%wZ\") -> 0x%08lX\n",
                RawDllName, Status);
        goto Quickie;
    }

    LDR_DDAG_STATE DdagState;

    Status = LdrpFindLoadedDllInternal(&DllName.String, PathSearchContext, Module, &DdagState, NewContextFlags);

    if (!NT_SUCCESS(Status))
        goto Quickie;

    if (NT_SUCCESS(Status) && DdagState < LdrModulesCondensed && !NtCurrentTeb()->LoadOwner)
    {
        // We have acquired DLL that is still in the process of being loaded. Wait a bit and retry.
        LdrpDereferenceModule(*Module);
        *Module = NULL;

        // Do the actual wait (by doing actual work)
        LdrpDrainWorkQueue(LoadComplete);

        Status = LdrpFindLoadedDllInternal(&DllName.String, PathSearchContext, Module, &DdagState, NewContextFlags);

        LdrpDropLastInProgressCount();

        if (NT_SUCCESS(Status) && DdagState != LdrModulesReadyToRun)
        {
            // Something has gone horribly wrong. Even after waiting we have found DLL which is in unexpected state.
            Status = STATUS_DLL_NOT_FOUND;
            LdrpDereferenceModule(*Module);
            *Module = NULL;
        }
    }

Quickie:

    LdrpFreeUnicodeStringBundle(DllName);

    return Status;
}
