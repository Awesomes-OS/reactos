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

#undef SearchPath

/* GLOBALS *******************************************************************/

BOOLEAN g_ShimsEnabled;
PVOID g_pShimEngineModule;
PVOID g_pfnSE_DllLoaded;
PVOID g_pfnSE_DllUnloaded;
PVOID g_pfnSE_InstallBeforeInit;
PVOID g_pfnSE_InstallAfterInit;
PVOID g_pfnSE_ProcessDying;

LIST_ENTRY LdrpWorkQueue;
LIST_ENTRY LdrpRetryQueue;
RTL_CRITICAL_SECTION LdrpWorkQueueLock;

/* FUNCTIONS *****************************************************************/


#if 0
ULONG
NTAPI
LdrpClearLoadInProgress(VOID)
{
    PLIST_ENTRY ListHead, Entry;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    ULONG ModulesCount = 0;

    /* Traverse the init list */
    ListHead = &NtCurrentPeb()->Ldr->InInitializationOrderModuleList;
    Entry = ListHead->Flink;
    while (Entry != ListHead)
    {
        /* Get the loader entry */
        LdrEntry = CONTAINING_RECORD(Entry,
                                     LDR_DATA_TABLE_ENTRY,
                                     InInitializationOrderLinks);

        /* Clear load in progress flag */
        LdrEntry->LoadInProgress = FALSE;

        /* Check for modules with entry point count but not processed yet */
        if (LdrEntry->EntryPoint && !LdrEntry->EntryProcessed)
        {
            /* Increase counter */
            ModulesCount++;
        }

        /* Advance to the next entry */
        Entry = Entry->Flink;
    }

    /* Return final count */
    return ModulesCount;
}
#endif

/* EOF */
