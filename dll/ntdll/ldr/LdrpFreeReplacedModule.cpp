#include <ldrp.h>

void
NTAPI
LdrpFreeReplacedModule(PLDR_DATA_TABLE_ENTRY Module)
{
    LDRP_ASSERT_MODULE_ENTRY(Module);

    LDRP_ASSERT_LOAD_CONTEXT(Module->LoadContext);
    ASSERT(Module->LoadContext->Module == Module);
    ASSERT(!Module->DdagNode->LoadCount);
    ASSERT(Module->ReferenceCount <= 2);
    ASSERT(!Module->DdagNode->Dependencies.Tail);

    LdrpFreeLoadContext(Module->LoadContext);
    Module->ReferenceCount = 1;
    Module->ProcessStaticImport = FALSE;
    LdrpDereferenceModule(Module);
}
