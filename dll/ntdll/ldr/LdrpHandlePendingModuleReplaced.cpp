#include <ldrp.h>

void
NTAPI
LdrpHandlePendingModuleReplaced(PLDRP_LOAD_CONTEXT LoadContext)
{
    LDRP_ASSERT_LOAD_CONTEXT(LoadContext);

    const PLDR_DATA_TABLE_ENTRY OldEntry = LoadContext->PendingModule;

    LDRP_ASSERT_MODULE_ENTRY_OPTIONAL(OldEntry);

    if (OldEntry)
    {
        const PLDR_DATA_TABLE_ENTRY NewEntry = LdrpHandleReplacedModule(OldEntry);

        LDRP_ASSERT_MODULE_ENTRY_OPTIONAL(NewEntry);

        if (NewEntry != OldEntry)
            LdrpFreeReplacedModule(OldEntry);

        LoadContext->PendingModule = NULL;
    }
}
