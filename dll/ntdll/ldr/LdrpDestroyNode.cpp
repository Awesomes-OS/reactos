#include <ldrp.h>
#include "tracing.hpp"
#include <reactos/ldrp.h>

void
NTAPI
LdrpDestroyNode(IN PLDR_DDAG_NODE Node)
{
    LDR_FUNC_VOID(Node)
        LDRP_ASSERT_DDAG_NODE(Node);

        ASSERT_FALSE(Node->LoadCount);
        ASSERT_FALSE((ULONG_PTR)Node->IncomingDependencies.Tail);
        ASSERT_FALSE((ULONG_PTR)Node->Dependencies.Tail);
        ASSERT_TRUE(Node->State <= LdrModulesPlaceHolder);

        LdrpHeapFree(0, Node);
    LDR_FUNC_END_VOID
}
