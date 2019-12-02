#include <ldrp.h>

void
NTAPI
LdrpLoadDependentModuleInternal(IN PUNICODE_STRING DllName,
                                IN PLDRP_LOAD_CONTEXT LoadContext,
                                IN LDRP_LOAD_CONTEXT_FLAGS LoadContextFlags,
                                IN LDR_DLL_LOAD_REASON LoadReason,
                                IN PLDR_DATA_TABLE_ENTRY ParentEntry,
                                OUT PLDR_DATA_TABLE_ENTRY* OutputLdrEntry,
                                IN OUT PLDRP_CSLIST_ENTRY* DependencyEntryPlaceholder,
                                OUT NTSTATUS* OutStatus)
{
    // Allocate PLDRP_CSLIST_ENTRY if needed
    if (!*DependencyEntryPlaceholder)
    {
        // We didn't have one preserved from previous iterations
        // (e.g. from previous IAT thunk loop iteration in LdrpMapAndSnapDependency)
        *DependencyEntryPlaceholder = LdrpHeapAlloc(HEAP_ZERO_MEMORY, sizeof(LDRP_CSLIST_ENTRY));

        if (!*DependencyEntryPlaceholder)
        {
            *OutStatus = STATUS_NO_MEMORY;
            return;
        }
    }

    const NTSTATUS FindStatus = LdrpFindOrPrepareLoadingModule(DllName,
                                                               LoadContext->PathSearchContext,
                                                               LoadContextFlags,
                                                               LoadReason,
                                                               ParentEntry,
                                                               OutputLdrEntry,
                                                               OutStatus);

    if (FindStatus == STATUS_DLL_NOT_FOUND)
    {
        LdrpProcessWork((*OutputLdrEntry)->LoadContext);
    }
    else if (!NT_SUCCESS(FindStatus) && FindStatus != STATUS_RETRY)
    {
        DPRINT1("LDR: LdrpFindOrPrepareLoadingModule(\"%wZ\", ...) -> 0x%08lX\n",
                DllName, FindStatus);
        *OutStatus = FindStatus;
    }

    if (*OutputLdrEntry)
    {
        // We might have replaced Entry during mapping (e.g. found already mapped entry)
        const PLDR_DATA_TABLE_ENTRY ReplacedEntry = LdrpHandleReplacedModule(*OutputLdrEntry);
        if (ReplacedEntry != *OutputLdrEntry)
        {
            LdrpFreeReplacedModule(*OutputLdrEntry);
            *OutputLdrEntry = ReplacedEntry;
        }

        if (NT_SUCCESS(*OutStatus))
        {
            *DependencyEntryPlaceholder = LdrpRecordModuleDependency(ParentEntry, *OutputLdrEntry, *DependencyEntryPlaceholder, OutStatus);
            // If result is null, dependency link didn't exist, but was created using PLDRP_CSLIST_ENTRY we've passed.
            // Otherwise it preserves DependencyEntryPlaceholder, which will be reused on next LdrpLoadDependentModule calls
            // or freed by caller when not needed anymore.
        }

        switch (*OutStatus)
        {
            case STATUS_DLL_NOT_FOUND:
                LdrpQueueWork((*OutputLdrEntry)->LoadContext);
                // fallthrough
            case STATUS_RETRY:
                *OutStatus = STATUS_SUCCESS;
                // fallthrough
            default:
                break;
        }

        if (NT_SUCCESS(*OutStatus))
        {
            if (LoadContext->CountOfDependenciesPendingMap)
            {
                if (*DependencyEntryPlaceholder || (*OutputLdrEntry)->DdagNode->State >= LdrModulesMapped)
                    LoadContext->CountOfDependenciesPendingMap--;
            }
            else if ((*OutputLdrEntry)->DdagNode->State < LdrModulesMapped)
            {
                LoadContext->PendingDependencyEntry = *OutputLdrEntry;
                // Stop the world, we've got to get the OutputLdrEntry before we can continue.
                LoadContext->CountOfDependenciesPendingMap = 1;
                ParentEntry->DdagNode->State = LdrModulesWaitingForDependencies;
                *OutStatus = STATUS_PENDING;
            }
        }

        LdrpDereferenceModule(*OutputLdrEntry);
    }
}

NTSTATUS
NTAPI
LdrpLoadDependentModule(IN PANSI_STRING RawDllName,
                        IN PLDRP_LOAD_CONTEXT LoadContext,
                        IN PLDR_DATA_TABLE_ENTRY ParentLdrEntry,
                        IN LDR_DLL_LOAD_REASON LoadReason,
                        OUT PLDR_DATA_TABLE_ENTRY* OutputLdrEntry,
                        IN OUT PLDRP_CSLIST_ENTRY* DependencyEntryPlaceholder)
{
    LDRP_UNICODE_STRING_BUNDLE DllName, RawDllNameUnicode;
    LDRP_LOAD_CONTEXT_FLAGS Flags = {0};
    NTSTATUS Status;

    LdrpCreateUnicodeStringBundle(DllName);
    LdrpCreateUnicodeStringBundle(RawDllNameUnicode);

    ASSERT(OutputLdrEntry);
    ASSERT(DependencyEntryPlaceholder);

    DPRINT("LDR: %s(\"%Z\", %p, %p[%wZ], %d, %p, %p)\n",
           __FUNCTION__,
           RawDllName,
           LoadContext,
           ParentLdrEntry,
           &ParentLdrEntry->BaseDllName,
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
                                           ParentLdrEntry->EntryPointActivationContext);

    Status = LdrpPreprocessDllName(&RawDllNameUnicode.String, &DllName, ParentLdrEntry, &Flags);

    if (NT_SUCCESS(Status))
    {
        LdrpLoadDependentModuleInternal(&DllName.String,
                                        LoadContext,
                                        Flags,
                                        LoadReason,
                                        ParentLdrEntry,
                                        OutputLdrEntry,
                                        DependencyEntryPlaceholder,
                                        LoadContext->StatusResponse);
    }

    RtlDeactivateActivationContextUnsafeFast(&ActCtx);

Quickie:

    if (!NT_SUCCESS(Status))
    {
        *OutputLdrEntry = NULL;
        *LoadContext->StatusResponse = Status;
    }

    LdrpFreeUnicodeStringBundle(DllName);
    LdrpFreeUnicodeStringBundle(RawDllNameUnicode);

    return Status;
}
