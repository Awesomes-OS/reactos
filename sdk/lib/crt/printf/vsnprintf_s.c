/*
 * COPYRIGHT:       GNU GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS crt library
 * FILE:            lib/sdk/crt/printf/vsnprintf_s.c
 * PURPOSE:         Implementation of vsnprintf_s
 */

#define _sxprintf vsnprintf_s
#define USE_COUNT 1
#define USE_VARARGS 1
#define IS_SECAPI 1

#include "_sxprintf.c"
