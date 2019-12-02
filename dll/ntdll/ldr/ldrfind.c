/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS NT User-Mode Library
 * FILE:            dll/ntdll/ldr/ldrutils.c
 * PURPOSE:         Internal Loader Utility Functions
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntdll.h>

//#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

PLDR_DATA_TABLE_ENTRY LdrpLoadedDllHandleCache;

BOOLEAN g_ShimsEnabled;
PVOID g_pShimEngineModule;
PVOID g_pfnSE_DllLoaded;
PVOID g_pfnSE_DllUnloaded;
PVOID g_pfnSE_InstallBeforeInit;
PVOID g_pfnSE_InstallAfterInit;
PVOID g_pfnSE_ProcessDying;

/* FUNCTIONS *****************************************************************/

/* EOF */
