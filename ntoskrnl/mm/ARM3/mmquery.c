/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/mmquery.c
 * PURPOSE:         ARM Memory Manager Information Query Routines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 *                  Andrew Boyarshin (andrew.boyarshin@gmail.com)
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define MODULE_INVOLVED_IN_ARM3
#include <mm/ARM3/miarm.h>

extern ULONG64 MiStandbyRepurposedByPriority[8];

NTSTATUS
NTAPI
MmQueryMemoryListInformation(IN OUT PSYSTEM_MEMORY_LIST_INFORMATION data, IN SIZE_T size)
{
    SIZE_T i;

    data->FreePageCount = MmFreePageListHead.Total;
    data->ZeroPageCount = MmZeroedPageListHead.Total;
    data->BadPageCount = 0;
    data->ModifiedPageCount = MmModifiedPageListHead.Total;
    data->ModifiedNoWritePageCount = MmModifiedNoWritePageListHead.Total;
    if (size > sizeof(SIZE_T) * (5 + 8 * 2))
    {
        data->ModifiedPageCountPageFile = 0;
    }

    /* Loop all 8 standby lists */
    for (i = 0; i < 8; i++)
    {
        data->PageCountByPriority[i] = MmStandbyPageListByPriority[i].Total;
        data->RepurposedPagesByPriority[i] = min(MiStandbyRepurposedByPriority[i], SIZE_MAX);
    }
    return STATUS_SUCCESS;
}
