#include <ldrp.h>

void
NTAPI
LdrpHandlePendingModuleReplaced(PLDRP_LOAD_CONTEXT LoadContext)
{
    const PLDR_DATA_TABLE_ENTRY OldEntry = LoadContext->PendingDependencyEntry;
    if (OldEntry)
    {
        const PLDR_DATA_TABLE_ENTRY NewEntry = LdrpHandleReplacedModule(OldEntry);

        if (NewEntry != OldEntry)
            LdrpFreeReplacedModule(OldEntry);

        LoadContext->PendingDependencyEntry = NULL;
    }
}
