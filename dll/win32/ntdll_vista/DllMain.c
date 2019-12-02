#include <stdarg.h>

#define WIN32_NO_STATUS

#include <windef.h>
#include <ndk/rtltypes.h>
#include <ndk/ldrfuncs.h>

#define NDEBUG
#include <debug.h>

VOID
RtlpInitializeKeyedEvent(VOID);

VOID
RtlpCloseKeyedEvent(VOID);

BOOL
WINAPI
DllMain(HANDLE hDll,
        DWORD dwReason,
        LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        LdrDisableThreadCalloutsForDll(hDll);
        RtlpInitializeKeyedEvent();
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        RtlpCloseKeyedEvent();
    }
    return TRUE;
}
