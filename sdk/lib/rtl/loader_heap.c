/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/rtl/loader.c
 * PURPOSE:         Shared RTL PE image loader functions (with heap allocation)
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

PVOID
NTAPI
LdrpRtlHeapAlloc(IN ULONG Flags OPTIONAL,
                 IN SIZE_T Size)
{
    return RtlAllocateHeap(RtlGetProcessHeap(),
                           Flags,
                           Size);
}

BOOLEAN
NTAPI
LdrpRtlHeapFree(IN ULONG Flags OPTIONAL,
                IN PVOID BaseAddress)
{
    return RtlFreeHeap(RtlGetProcessHeap(), Flags, BaseAddress);
}

PVOID
NTAPI
LdrpRtlHeapReAlloc(IN ULONG Flags OPTIONAL,
                   IN PVOID Ptr,
                   IN SIZE_T Size)
{
    return RtlReAllocateHeap(RtlGetProcessHeap(),
                             Flags,
                             Ptr,
                             Size);
}