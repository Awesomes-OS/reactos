#include "advapi32_vista.h"

#define _EVNT_SOURCE_
#include <evntprov.h>
#include <wine/debug.h>
#include <wine/unicode.h>

WINE_DEFAULT_DEBUG_CHANNEL(advapi);

/******************************************************************************
 * EventProviderEnabled [ADVAPI32.@]
 *
 */
BOOLEAN WINAPI EventProviderEnabled( REGHANDLE handle, UCHAR level, ULONGLONG keyword )
{
    FIXME("%s, %u, %s: stub\n", wine_dbgstr_longlong(handle), level, wine_dbgstr_longlong(keyword));
    return FALSE;
}

/******************************************************************************
 * EventActivityIdControl [ADVAPI32.@]
 *
 */
ULONG WINAPI EventActivityIdControl(ULONG code, GUID *guid)
{
    static int once;

    if (!once++) FIXME("0x%x, %p: stub\n", code, guid);
    return ERROR_SUCCESS;
}

/******************************************************************************
 * EventWriteTransfer [ADVAPI32.@]
 */
ULONG WINAPI EventWriteTransfer( REGHANDLE handle, PCEVENT_DESCRIPTOR descriptor, LPCGUID activity,
                                 LPCGUID related, ULONG count, PEVENT_DATA_DESCRIPTOR data )
{
    FIXME("%s, %p, %s, %s, %u, %p: stub\n", wine_dbgstr_longlong(handle), descriptor,
          debugstr_guid(activity), debugstr_guid(related), count, data);
    return ERROR_SUCCESS;
}