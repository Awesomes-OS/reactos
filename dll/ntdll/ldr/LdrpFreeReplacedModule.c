#include <ldrp.h>

void
NTAPI
LdrpFreeReplacedModule(PLDR_DATA_TABLE_ENTRY Entry)
{
    LdrpFreeLoadContext(Entry->LoadContext);
    Entry->ReferenceCount = 1;
    Entry->ProcessStaticImport = FALSE;
    LdrpDereferenceModule(Entry);
}