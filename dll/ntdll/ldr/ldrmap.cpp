/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS NT User-Mode Library
 * FILE:            dll/ntdll/ldr/ldrutils.c
 * PURPOSE:         Internal Loader Utility Functions
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ldrp.h>

/* GLOBALS *******************************************************************/

LIST_ENTRY LdrpWorkQueue;
LIST_ENTRY LdrpRetryQueue;
RTL_CRITICAL_SECTION LdrpWorkQueueLock;

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
LdrpMapDllRetry(IN PLDRP_LOAD_CONTEXT LoadContext)
{
    NTSTATUS Status;
    PLDR_DATA_TABLE_ENTRY NewEntry;
    LDRP_UNICODE_STRING_BUNDLE NtFileName;

    LdrpCreateUnicodeStringBundle(NtFileName);

    Status = LdrpFindExistingModule(&LoadContext->Module->BaseDllName,
                                    &LoadContext->Module->FullDllName,
                                    LoadContext->Flags,
                                    LoadContext->Module->BaseNameHashValue,
                                    &NewEntry);

    if (NewEntry)
    {
        LdrpLoadContextReplaceModule(LoadContext, NewEntry);
    }
    else
    {
        Status = LdrpGetNtPathFromDosPath(&LoadContext->Module->FullDllName, &NtFileName);

        if (NT_SUCCESS(Status))
            Status = LdrpMapDllNtFileName(LoadContext, &NtFileName.String);
    }

    LdrpFreeUnicodeStringBundle(NtFileName);

    return Status;
}

/* EOF */
