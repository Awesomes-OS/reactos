#include <ldrp.h>
#include <reactos/ldrp.h>

void
NTAPI
LdrpDestroyNode(IN PLDR_DDAG_NODE Node)
{
    ASSERT(Node);

    LdrpHeapFree(0, Node);
}