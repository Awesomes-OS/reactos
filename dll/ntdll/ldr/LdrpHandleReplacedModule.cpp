#include <ldrp.h>

PLDR_DATA_TABLE_ENTRY
NTAPI
LdrpHandleReplacedModule(PLDR_DATA_TABLE_ENTRY Entry)
{
    PLDR_DATA_TABLE_ENTRY ReplacedModule = Entry;

    if (Entry)
    {
        const PLDRP_LOAD_CONTEXT LoadContext = Entry->LoadContext;

        if (LoadContext && LoadContext->Module != Entry)
        {
            ASSERT(LoadContext->Module);

            ReplacedModule = LoadContext->Module;

            LoadContext->Module = Entry;

            ASSERT(!ReplacedModule->LoadContext || ReplacedModule == ReplacedModule->LoadContext->Module);
        }
    }

    return ReplacedModule;
}
