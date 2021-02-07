#include <ldrp.h>

NTSTATUS
NTAPI
LdrpLoadDependentModuleInternal(IN PUNICODE_STRING DllName,
                                IN PLDRP_LOAD_CONTEXT LoadContext,
                                IN LDRP_LOAD_CONTEXT_FLAGS LoadContextFlags,
                                IN LDR_DLL_LOAD_REASON LoadReason,
                                IN PLDR_DATA_TABLE_ENTRY ParentModule,
                                OUT PLDR_DATA_TABLE_ENTRY* OutputLdrEntry,
                                IN OUT PLDRP_CSLIST_ENTRY* DependencyEntryPlaceholder)
{
    LDRP_ASSERT_LOAD_CONTEXT(LoadContext);
    LDRP_ASSERT_MODULE_ENTRY(ParentModule);
    ASSERT(OutputLdrEntry);
    ASSERT(DependencyEntryPlaceholder);

    // Allocate PLDRP_CSLIST_ENTRY if needed
    if (!*DependencyEntryPlaceholder)
    {
        // We didn't have one preserved from previous iterations
        // (e.g. from previous IAT thunk loop iteration in LdrpMapAndSnapDependency)
        *DependencyEntryPlaceholder = new LDRP_CSLIST_ENTRY;

        if (!*DependencyEntryPlaceholder)
        {
            return STATUS_NO_MEMORY;
        }

        LDRP_MARK_CSLIST_ENTRY(*DependencyEntryPlaceholder);
    }

    NTSTATUS Status = LdrpFindOrPrepareLoadingModule(DllName,
                                                     LoadContext->PathSearchContext,
                                                     LoadContextFlags,
                                                     LoadReason,
                                                     ParentModule,
                                                     OutputLdrEntry,
                                                     LoadContext->LoadStatus);

    if (*OutputLdrEntry)
    {
        if (Status == STATUS_DLL_NOT_FOUND && LoadContextFlags.RetryDependentLoadWhenMissing)
        {
            const PLDRP_LOAD_CONTEXT EntryLoadContext = (*OutputLdrEntry)->LoadContext;
            LdrpProcessWork(EntryLoadContext, StandaloneRequest);
            Status = *EntryLoadContext->LoadStatus;
        }

        RtlAcquireSRWLockExclusive(&LdrpModuleDatatableLock);

        // We might have replaced Entry during mapping (e.g. found already mapped entry)
        const PLDR_DATA_TABLE_ENTRY ExportModule = LdrpHandleReplacedModule(*OutputLdrEntry);

        *DependencyEntryPlaceholder = LdrpRecordModuleDependency(LoadContext->Module, ExportModule,
                                                                 *DependencyEntryPlaceholder, &Status);

        // If result is null, dependency link didn't exist, but was created using PLDRP_CSLIST_ENTRY we've passed.
        // Otherwise it preserves DependencyEntryPlaceholder, which will be reused on next LdrpLoadDependentModule calls
        // or freed by caller when not needed anymore.

        switch (Status)
        {
            case STATUS_DLL_NOT_FOUND:
                if (!LoadContextFlags.RetryDependentLoadWhenMissing)
                {
                    LdrpQueueWork(ExportModule->LoadContext);
                    Status = STATUS_SUCCESS;
                }
                break;
            case STATUS_RETRY:
                Status = STATUS_SUCCESS;
                break;

            default: ;
        }

        if (NT_SUCCESS(Status))
        {
            if (LoadContext->UnmappedChildCount)
            {
                ASSERT(LoadContext->Module->DdagNode->State < LdrModulesSnapping);

                if (*DependencyEntryPlaceholder || ExportModule->DdagNode->State >= LdrModulesMapped)
                {
                    ASSERT(LoadContext->UnmappedChildCount > 1);
                    LoadContext->UnmappedChildCount--;
                }
            }
            else
            {
                ASSERT(LoadContext->Module->DdagNode->State == LdrModulesSnapping);

                if (ExportModule->DdagNode->State < LdrModulesMapped)
                {
                    LoadContext->PendingModule = ExportModule;
                    // Stop the world, we've got to get the OutputLdrEntry before we can continue.
                    LoadContext->UnmappedChildCount = 1;
                    LoadContext->Module->DdagNode->State = LdrModulesWaitingForDependencies;
                    Status = STATUS_PENDING;
                }
            }
        }

        LdrpDereferenceModule(ExportModule);

        RtlReleaseSRWLockExclusive(&LdrpModuleDatatableLock);

        if (ExportModule != *OutputLdrEntry)
        {
            LdrpFreeReplacedModule(*OutputLdrEntry);
            *OutputLdrEntry = ExportModule;
        }
    }

    return Status;
}

NTSTATUS
NTAPI
LdrpLoadDependentModule(IN PANSI_STRING RawDllName,
                        IN PLDRP_LOAD_CONTEXT LoadContext,
                        IN PLDR_DATA_TABLE_ENTRY ParentModule,
                        IN LDR_DLL_LOAD_REASON LoadReason,
                        OUT PLDR_DATA_TABLE_ENTRY* OutputLdrEntry,
                        IN OUT PLDRP_CSLIST_ENTRY* DependencyEntryPlaceholder)
{
    LDRP_UNICODE_STRING_BUNDLE DllName, RawDllNameUnicode;
    LDRP_LOAD_CONTEXT_FLAGS Flags = {0};
    NTSTATUS Status;

    LdrpCreateUnicodeStringBundle(DllName);
    LdrpCreateUnicodeStringBundle(RawDllNameUnicode);

    ASSERT(RawDllName);
    ASSERT(LoadContext);
    ASSERT(ParentModule);
    ASSERT(OutputLdrEntry);
    ASSERT(DependencyEntryPlaceholder);

    DPRINT("LDR: %s(\"%Z\", " LDR_HEXPTR_FMT ", " LDR_HEXPTR_FMT "[%wZ], %d, " LDR_HEXPTR_FMT ", " LDR_HEXPTR_FMT ")\n",
           __FUNCTION__,
           RawDllName,
           LoadContext,
           ParentModule,
           &ParentModule->BaseDllName,
           LoadReason,
           OutputLdrEntry,
           *DependencyEntryPlaceholder);

    Status = LdrpAppendAnsiStringToFilenameBuffer(&RawDllNameUnicode, RawDllName);
    if (!NT_SUCCESS(Status))
        goto Quickie;

    RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_EXTENDED ActCtx;

    // Set up and activate the Activation Context based upon ParentEntry
    RtlZeroMemory(&ActCtx, sizeof(ActCtx));
    ActCtx.Size = sizeof(ActCtx);
    ActCtx.Format = RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_FORMAT_WHISTLER;

    RtlActivateActivationContextUnsafeFast(&ActCtx,
                                           ParentModule->EntryPointActivationContext);

    Status = LdrpPreprocessDllName(&RawDllNameUnicode.String, &DllName, ParentModule, &Flags);

    if (NT_SUCCESS(Status))
    {
        Status = LdrpLoadDependentModuleInternal(&DllName.String,
                                                 LoadContext,
                                                 Flags,
                                                 LoadReason,
                                                 ParentModule,
                                                 OutputLdrEntry,
                                                 DependencyEntryPlaceholder);
    }

    RtlDeactivateActivationContextUnsafeFast(&ActCtx);

Quickie:

    if (NT_SUCCESS(Status))
    {
        LDRP_ASSERT_MODULE_ENTRY(*OutputLdrEntry);
    }
    else
    {
        DPRINT1("LDR: LdrpLoadDependentModuleInternal(\"%wZ\", ...) -> 0x%08lX\n",
                &DllName.String, Status);

        *OutputLdrEntry = NULL;
        *LoadContext->LoadStatus = Status;
    }

    LdrpFreeUnicodeStringBundle(DllName);
    LdrpFreeUnicodeStringBundle(RawDllNameUnicode);

    return Status;
}
