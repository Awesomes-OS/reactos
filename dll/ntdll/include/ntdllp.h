/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            dll/ntdll/include/ntdllp.h
 * PURPOSE:         Native Library Internal Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

#pragma once
#include <ndk/umtypes.h>

/* path.c */
BOOLEAN
NTAPI
RtlDoesFileExists_UStr(
    IN PUNICODE_STRING FileName
);

/* EOF */
