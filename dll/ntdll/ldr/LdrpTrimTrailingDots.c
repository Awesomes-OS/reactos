#include <ldrp.h>

void
NTAPI
LdrpTrimTrailingDots(IN OUT PUNICODE_STRING DllName)
{
    WCHAR* p = DllName->Buffer + (DllName->Length / sizeof(WCHAR)) - 1;
    for (; p >= DllName->Buffer; p--, DllName->Length -= sizeof(WCHAR))
    {
        if (*p != L'.')
            break;
    }
}