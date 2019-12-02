#include <ldrp.h>
#include <ntintsafe.h>

UINT32
NTAPI
LdrpNameToOrdinal(IN LPSTR ImportName,
                  IN ULONG NumberOfNames,
                  IN PVOID ExportBase,
                  IN PULONG NameTable,
                  IN PUSHORT OrdinalTable)
{
    LONG Start, End;

    /* Use classical binary search to find the ordinal */
    Start = 0;
    End = NumberOfNames - 1;
    while (Start <= End)
    {
        /* Next will be exactly between Start and End */
        LONG Next = (Start + End) / 2;

        /* Compare this name with the one we need to find */
        int CmpResult = strcmp(ImportName, PTR_ADD_OFFSET(ExportBase, NameTable[Next]));

        /* We found our entry if result is 0 */
        if (!CmpResult)
            return OrdinalTable[Next];

        /* We didn't find, update our range then */
        if (CmpResult < 0)
            End = Next - 1;
        else if (CmpResult > 0)
            Start = Next + 1;
    }

    // The search failed

    if (LdrpDebugFlags.LogWarning)
        DPRINT1("LDR: %s(\"%s\", %lu, %p, ...) -> -1\n", __FUNCTION__, ImportName, NumberOfNames, ExportBase);

    if (LdrpDebugFlags.BreakInDebugger)
        __debugbreak();

    return UINT32_MAX;
}