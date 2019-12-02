#include <ldrp.h>

void
NTAPI
LdrpFreeLoadContext(PLDRP_LOAD_CONTEXT LoadContext)
{
    LoadContext->Entry->LoadContext = NULL;

    LdrpHandlePendingModuleReplaced(LoadContext);

    for (ULONG Index = 0; Index < LoadContext->ImportEntriesCount; Index++)
    {
        const PLDR_DATA_TABLE_ENTRY LdrEntry = LoadContext->ImportEntries[Index];

        if (!LdrEntry)
            continue;

        const PLDRP_LOAD_CONTEXT LdrEntryContext = LdrEntry->LoadContext;

        if (!LdrEntryContext)
            continue;

        if (LdrEntryContext->Entry == LdrEntry)
            continue;

        LdrEntryContext->Entry = LdrEntry;
        LdrpFreeReplacedModule(LdrEntry);
    }

    if (LoadContext->ImportEntries)
        LdrpHeapFree(0, LoadContext->ImportEntries);

    LdrpHeapFree(0, LoadContext);
}
