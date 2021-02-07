#include <ldrp.h>

BOOLEAN
NTAPI
LdrpIsExtensionPresent(IN PUNICODE_STRING DllName)
{
    WCHAR *p = DllName->Buffer + (DllName->Length / sizeof(WCHAR)) - 1;
    while (p >= DllName->Buffer)
    {
        const WCHAR c = *p--;
        if (c == L'.')
            return TRUE;
        if (c == L'\\' || c == L'/')
            return FALSE;
    }
    return FALSE;
}