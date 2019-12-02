#include <ldrp.h>
#include <reactos/ldrp.h>

NTSTATUS
NTAPI
LdrpAllocatePlaceHolder(IN PUNICODE_STRING DllName,
                        IN PLDRP_PATH_SEARCH_CONTEXT PathSearchContext,
                        IN LDRP_LOAD_CONTEXT_FLAGS Flags,
                        IN LDR_DLL_LOAD_REASON LoadReason,
                        IN PLDR_DATA_TABLE_ENTRY ParentEntry,
                        OUT PLDR_DATA_TABLE_ENTRY* OutLdrEntry,
                        IN NTSTATUS* OutStatus)
{
    NTSTATUS Status = STATUS_SUCCESS;

    /* Sanity checks */
    ASSERT(OutLdrEntry);

    /* Allocate the struct */
    PLDRP_LOAD_CONTEXT LoadContext = LdrpAllocateHeap(HEAP_ZERO_MEMORY,
                                                      FIELD_OFFSET(LDRP_LOAD_CONTEXT,
                                                                   DllNameStorage[DllName->Length]));
    if (!LoadContext)
    {
        Status = STATUS_NO_MEMORY;
        goto Quickie;
    }

    LoadContext->ParentEntry = ParentEntry;
    LoadContext->StatusResponse = OutStatus;
    LoadContext->Flags = Flags;
    LoadContext->Flags.UserAllocated = TRUE;
    LoadContext->PathSearchContext = PathSearchContext;

    LoadContext->DllName.Buffer = LoadContext->DllNameStorage;
    LoadContext->DllName.MaximumLength = DllName->Length + sizeof(WCHAR);
    RtlCopyUnicodeString(&LoadContext->DllName, DllName);

    if (!((LoadContext->Entry = LdrpAllocateModuleEntry(LoadContext))))
    {
        Status = STATUS_NO_MEMORY;
        LdrpFreeHeap(0, LoadContext);
        LoadContext = NULL;
        goto Quickie;
    }

    LoadContext->Entry->LoadReason = LoadReason;

Quickie:
    *OutLdrEntry = LoadContext ? LoadContext->Entry : NULL;

    return Status;
}

PLDR_DATA_TABLE_ENTRY
NTAPI
LdrpAllocateModuleEntry(IN PLDRP_LOAD_CONTEXT LoadContext OPTIONAL)
{
    /* Allocate an entry */
    const PLDR_DATA_TABLE_ENTRY LdrEntry = LdrpAllocateHeap(HEAP_ZERO_MEMORY,
                                                            sizeof(LDR_DATA_TABLE_ENTRY));

    /* Make sure we got one */
    if (!LdrEntry)
    {
        return NULL;
    }

    /* Allocate a corresponding DDAG node */
    const PLDR_DDAG_NODE Node = LdrpAllocateHeap(HEAP_ZERO_MEMORY,
                                                 sizeof(LDR_DDAG_NODE));

    /* Make sure we got one */
    if (!Node)
    {
        LdrpHeapFree(0, LdrEntry);
        return NULL;
    }

    LdrEntry->DdagNode = Node;

    LdrEntry->LoadContext = LoadContext;
    if (LoadContext)
    {
        LoadContext->Entry = LdrEntry;

        if (LoadContext->ParentEntry)
        {
            if (LoadContext->ParentEntry->EntryPointActivationContext)
            {
                RtlAddRefActivationContext(LoadContext->ParentEntry->EntryPointActivationContext);
                LdrEntry->EntryPointActivationContext = LoadContext->ParentEntry->EntryPointActivationContext;
            }
        }
        else
        {
            const NTSTATUS Status = RtlGetActiveActivationContext(&LdrEntry->EntryPointActivationContext);

            if (!NT_SUCCESS(Status))
            {
                DPRINT1("LDR: RtlGetActiveActivationContext(...) -> 0x%08lX\n", Status);
            }

            LdrEntry->LoadReason = LoadReasonDynamicLoad;
        }
    }

    Node->LoadCount = 1u;
    LdrEntry->ReferenceCount = 2u;

    InitializeListHead(&LdrEntry->HashLinks);

    LdrEntry->NodeModuleLink.Flink = LdrEntry->NodeModuleLink.Blink = &Node->Modules;
    Node->Modules.Flink = Node->Modules.Blink = &LdrEntry->NodeModuleLink;

    LdrEntry->ObsoleteLoadCount = 12u; // random big enough value, to prevent misuse

    if (LdrpImageEntry)
    {
        const LDR_DDAG_STATE ImageState = LdrpImageEntry->DdagNode->State;

        if (ImageState >= LdrModulesMapped && ImageState <= LdrModulesSnapped)
        {
            LdrEntry->ProcessStaticImport = TRUE;
        }
    }

    if (LoadContext && LoadContext->Flags.Redirected)
        LdrEntry->Redirected = TRUE;

    return LdrEntry;
}
