#include <ldrp.h>

extern PTEB LdrpTopLevelDllBeingLoadedTeb; // defined in rtlsupp.c!

NTSTATUS
NTAPI
LdrpInitializeNode(IN PLDR_DDAG_NODE DdagNode)
{
    PPEB Peb = NtCurrentPeb();
    NTSTATUS Status = STATUS_SUCCESS;

    ASSERT(DdagNode->State >= LdrModulesSnapped);

    DdagNode->State = LdrModulesInitializing;

    const PLIST_ENTRY List = &Peb->Ldr->InInitializationOrderModuleList;

    LIST_ENTRY* const Head = &DdagNode->Modules;
    for (LIST_ENTRY* Current = Head->Blink; Current != Head; Current = Current->Blink)
    {
        const PLDR_DDAG_NODE CurrentNode = CONTAINING_RECORD(Current, LDR_DDAG_NODE, Modules);
        const PLDR_DATA_TABLE_ENTRY CurrentEntry = CONTAINING_RECORD(CurrentNode, LDR_DATA_TABLE_ENTRY, NodeModuleLink);

        if (CurrentEntry == LdrpImageEntry)
            continue;

        RtlpCheckListEntry(List);

        // Insert current entry into the PEB list
        InsertTailList(List, &CurrentEntry->InInitializationOrderLinks);
    }

    // Set the TLD TEB
    // The last Windows to set it is Windows 8.1.
    // Windows 10 uses a different approach for RtlIsThreadWithinLoaderCallout implementation.
    const PTEB OldTldTeb = LdrpTopLevelDllBeingLoadedTeb;
    LdrpTopLevelDllBeingLoadedTeb = NtCurrentTeb();

    // Run initialization routines for each module
    for (LIST_ENTRY* Current = Head->Blink; Current != Head; Current = Current->Blink)
    {
        const PLDR_DDAG_NODE CurrentNode = CONTAINING_RECORD(Current, LDR_DDAG_NODE, Modules);
        const PLDR_DATA_TABLE_ENTRY LdrEntry = CONTAINING_RECORD(CurrentNode, LDR_DATA_TABLE_ENTRY, NodeModuleLink);

        if (LdrEntry == LdrpImageEntry)
            continue;

        /* Run the init routine */
        Status = LdrpRunInitializeRoutine(LdrEntry);
        if (!NT_SUCCESS(Status))
        {
            /* Failed, unload the DLL */
            if (ShowSnaps)
            {
                DPRINT1("LDR: Unloading %wZ because either its init "
                        "routine or one of its static imports failed; "
                        "status = 0x%08lx\n",
                        &LdrEntry->BaseDllName,
                        Status);
            }
        }
    }

    DdagNode->State = NT_SUCCESS(Status) ? LdrModulesReadyToRun : LdrModulesInitError;

    /* Restore old TEB */
    LdrpTopLevelDllBeingLoadedTeb = OldTldTeb;

    return Status;
}