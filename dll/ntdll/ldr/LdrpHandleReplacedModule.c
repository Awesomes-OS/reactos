#include <ldrp.h>

PLDR_DATA_TABLE_ENTRY
NTAPI
LdrpHandleReplacedModule(PLDR_DATA_TABLE_ENTRY Entry)
{
    PLDR_DATA_TABLE_ENTRY Result = Entry;
    if (Entry && Entry->LoadContext && Entry->LoadContext->Entry != Entry)
    {
        Result = Entry->LoadContext->Entry;
    }

    return Result;
}