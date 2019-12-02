/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Vista+ NLS implementation
 */

#include "k32_vista.h"

#include <stdarg.h>
#include <winnls.h>

#define STUB \
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED); \
  DPRINT1("%s() is UNIMPLEMENTED!\n", __FUNCTION__)

#define NDEBUG
#include <debug.h>

/*
 * @unimplemented
 */
BOOL
WINAPI
GetNLSVersionEx(
    IN NLS_FUNCTION Function,
    IN LPCWSTR lpLocaleName,
    IN OUT LPNLSVERSIONINFOEX lpVersionInformation)
{
    STUB;
    return TRUE;
}