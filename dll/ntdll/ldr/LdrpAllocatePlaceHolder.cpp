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
    LDR_FUNC(NTSTATUS, DllName, PathSearchContext, Flags, LoadReason, ParentEntry, OutLdrEntry, OutStatus)
        NTSTATUS Status = STATUS_SUCCESS;

        /* Sanity checks */
        ASSERT_TRUE(OutLdrEntry);

        /* Allocate the struct */
        auto* LoadContext = (PLDRP_LOAD_CONTEXT)LdrpAllocateHeap(
            HEAP_ZERO_MEMORY,
            FIELD_OFFSET(LDRP_LOAD_CONTEXT, DllNameStorage[DllName->Length])
        );

        if (!LoadContext)
        {
            Status = STATUS_NO_MEMORY;
            goto Quickie;
        }

        LDRP_MARK_LOAD_CONTEXT(LoadContext);

        LoadContext->ParentModule = ParentEntry;
        LoadContext->LoadStatus = OutStatus;
        LoadContext->Flags = Flags;
        LoadContext->Flags.UserAllocated = TRUE;
        LoadContext->PathSearchContext = PathSearchContext;

        LoadContext->DllName.Buffer = LoadContext->DllNameStorage;
        LoadContext->DllName.MaximumLength = DllName->Length + sizeof(WCHAR);
        RtlCopyUnicodeString(&LoadContext->DllName, DllName);

        if (!((LoadContext->Module = LdrpAllocateModuleEntry(LoadContext))))
        {
            Status = STATUS_NO_MEMORY;
            LdrpFreeHeap(0, LoadContext);
            LoadContext = NULL;
            goto Quickie;
        }

        LoadContext->Module->LoadReason = LoadReason;

    Quickie:
        *OutLdrEntry = LoadContext ? LoadContext->Module : NULL;

        return Status;
    LDR_FUNC_END_RETURN
}

PLDR_DATA_TABLE_ENTRY
NTAPI
LdrpAllocateModuleEntry(IN PLDRP_LOAD_CONTEXT LoadContext OPTIONAL)
{
    LDR_FUNC(PLDR_DATA_TABLE_ENTRY, LoadContext)
        LDRP_ASSERT_LOAD_CONTEXT_OPTIONAL(LoadContext);

        /* Allocate an entry */
        auto* Module = (PLDR_DATA_TABLE_ENTRY)LdrpAllocateHeap(HEAP_ZERO_MEMORY,
                                                               sizeof(LDR_DATA_TABLE_ENTRY));

        /* Make sure we got one */
        if (!Module)
        {
            return NULL;
        }

        LDRP_MARK_MODULE_ENTRY(Module);

        /* Allocate a corresponding DDAG node */
        auto* Node = (PLDR_DDAG_NODE)LdrpAllocateHeap(HEAP_ZERO_MEMORY,
                                                      sizeof(LDR_DDAG_NODE));

        /* Make sure we got one */
        if (!Node)
        {
            LdrpHeapFree(0, Module);
            return NULL;
        }

        LDRP_MARK_DDAG_NODE(Node);

        Module->DdagNode = Node;

        Module->LoadContext = LoadContext;
        if (LoadContext)
        {
            LoadContext->Module = Module;

            if (LoadContext->ParentModule)
            {
                if (LoadContext->ParentModule->EntryPointActivationContext)
                {
                    RtlAddRefActivationContext(LoadContext->ParentModule->EntryPointActivationContext);
                    Module->EntryPointActivationContext = LoadContext->ParentModule->EntryPointActivationContext;
                }
            }
            else
            {
                const NTSTATUS Status = RtlGetActiveActivationContext(&Module->EntryPointActivationContext);

                ASSERT_SUCCESS_OR_IGNORE(Status);

                Module->LoadReason = LoadReasonDynamicLoad;
            }
        }

        Node->LoadCount = 1u;
        Module->ReferenceCount = 2u;

        InitializeListHead(&Module->HashLinks);

        Module->NodeModuleLink.Flink = Module->NodeModuleLink.Blink = &Node->Modules;
        Node->Modules.Flink = Node->Modules.Blink = &Module->NodeModuleLink;

        Module->ObsoleteLoadCount = 12u; // random non-special (-1, 0) non-1 value, to prevent misuse

        if (LdrInitState == 1)
            Module->ProcessStaticImport = TRUE;

        if (LoadContext)
        {
            if (LoadContext->Flags.Redirected)
                Module->Redirected = TRUE;

            if (!LoadContext->Flags.ExecutableImage)
                Module->ImageDll = TRUE;
        }

        return Module;
    LDR_FUNC_END_IMPL(return nullptr;);
}
