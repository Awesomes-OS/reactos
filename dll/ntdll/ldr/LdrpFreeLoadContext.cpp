#include <ldrp.h>

void
NTAPI
LdrpFreeLoadContext(PLDRP_LOAD_CONTEXT LoadContext)
{
    LDRP_ASSERT_LOAD_CONTEXT(LoadContext);

    LoadContext->Module->LoadContext = NULL;

    LdrpHandlePendingModuleReplaced(LoadContext);

    if (LoadContext->ImportArray)
    {
        for (ULONG Index = 0; Index < LoadContext->ImportCount; Index++)
        {
            const PLDR_DATA_TABLE_ENTRY Module = LoadContext->ImportArray[Index];

            if (!Module)
                continue;

            if (LdrpHandleReplacedModule(Module) == Module)
                continue;

            LdrpFreeReplacedModule(Module);
        }

        LdrpHeapFree(0, LoadContext->ImportArray);
    }

    LdrpHeapFree(0, LoadContext);
}
